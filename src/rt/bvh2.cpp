// ======================================================================== //
// Copyright (C) 2011 Benjamin Segovia                                      //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "bvh2.hpp"
#include "bvh2_node.hpp"
#include "rt_triangle.hpp"

#include "simd/ssef.hpp"
#include "math/bbox.hpp"
#include "sys/logging.hpp"
#include "sys/logging.hpp"
#include "sys/alloc.hpp"
#include "sys/tasking.hpp"

#include <algorithm>
#include <functional>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <vector>

namespace pf
{
  /*! Computes half surface area of box. */
  INLINE float halfArea(const Box& box) {
    const ssef d = size(box);
    const ssef a = d*shuffle<1,2,0,3>(d);
    return extract<0>(reduce_add(a));
  }

  /*! Be aware with the unused w component */
  INLINE Box convertBox(const BBox3f &from) {
    Box to;
    for (size_t j = 0; j < 3; ++j) to.lower[j] = from.lower[j];
    for (size_t j = 0; j < 3; ++j) to.upper[j] = from.upper[j];
    to.upper[3] = to.lower[3] = 0.f;
    return to;
  }

  /* Just the barycenter of a triangle */
  struct Centroid : public vec3f {
    Centroid(void) {}
    Centroid(const RTTriangle &t) :
      vec3f(t.v[0].x + t.v[1].x + t.v[2].x,
            t.v[0].y + t.v[1].y + t.v[2].y,
            t.v[0].z + t.v[1].z + t.v[2].z) {}
  };

  /*! To sort primitives */
  enum { ON_LEFT = 0, ON_RIGHT = 1 };

  /*! Used when sorting along each axis */
  static const int remapOtherAxis[4] = {1, 2, 0, 1};
  enum {otherAxisNum = 2};

  /*! n.log(n) compiler with bounding box sweeping and SAH */
  struct BVH2Builder : public RefCount
  {
    BVH2Builder(void);
    ~BVH2Builder(void);
    /*! Compute primitives centroids and sort then for each axis */
    template <typename T>
    void injection(const T * const RESTRICT soup, uint32 primNum);
    /*! Build the hierarchy itself */
    void compile(void);

    Box sceneAABB;              //!< AABB of the scene
    std::vector<uint32> primID; //!< Sorted array of primitives
    uint32 *IDs[3];             //!< Sorted ID per axis
    int32  *pos;                //!< Says if a node is on left or on right of the cut
    uint32 *tmpIDs;             //!< Used to temporaly store IDs
    Box *aabbs;                 //!< All the bounding boxes
    Box *rlAABBs;               //!< Bounding boxes sorted right to left
    BVH2Node * RESTRICT root;   //!< Root of the tree
    int32 n;                    //!< NUmber of primitives
    int32 nodeNum;              //!< Maximum number of nodes (== 2*n+1 for a BVH)
    uint32 currID;              //!< Last node pushed
    BVH2BuildOption options;
  };

  BVH2Builder::BVH2Builder(void) :
    pos(NULL), tmpIDs(NULL),
    aabbs(NULL), rlAABBs(NULL), root(NULL),
    n(0), nodeNum(0), currID(0)
  {
    for (size_t i = 0; i < 3; ++i) IDs[i] = NULL;
  }

  BVH2Builder::~BVH2Builder(void) {
    PF_ALIGNED_FREE(rlAABBs);
    PF_ALIGNED_FREE(aabbs);
    PF_ALIGNED_FREE(tmpIDs);
    PF_ALIGNED_FREE(pos);
    for (size_t i = 0; i < 3; ++i) PF_ALIGNED_FREE(IDs[i]);
  }

  /*! Sort the centroids along the given axis */
  template<uint32 axis>
  struct CentroidSorter : public std::binary_function<int, int, bool> {
    const Centroid *centroids;
    CentroidSorter(const Centroid *c) : centroids(c) {}
    INLINE int operator() (const uint32 a, const uint32 b) const {
      return centroids[a][axis] <  centroids[b][axis];
    }
  };

  template <typename T>
  void BVH2Builder::injection(const T * const RESTRICT soup, uint32 primNum)
  {
    Centroid *centroids = NULL;
    double t = getSeconds();

    // Allocate nodes and leaves for the BVH2
    nodeNum = 2 * primNum + 1;
    root = (BVH2Node*) PF_ALIGNED_MALLOC(nodeNum * sizeof(BVH2Node), CACHE_LINE);

    // Allocate the data for the compiler
    for (int i = 0; i < 3; ++i)
      IDs[i] = (uint32*) PF_ALIGNED_MALLOC(sizeof(uint32) * primNum, CACHE_LINE);
    tmpIDs = (uint32*) PF_ALIGNED_MALLOC(sizeof(uint32) * primNum, CACHE_LINE);
    pos = (int32*) PF_ALIGNED_MALLOC(sizeof(int32) * primNum, CACHE_LINE);
    aabbs = (Box*) PF_ALIGNED_MALLOC(sizeof(Box) * primNum, CACHE_LINE);
    rlAABBs = (Box*) PF_ALIGNED_MALLOC(sizeof(Box) * primNum, CACHE_LINE);
    centroids = (Centroid*) PF_ALIGNED_MALLOC(sizeof(Centroid) * primNum, CACHE_LINE);
    n = primNum;

    // Compute centroids and bounding boxes
    sceneAABB = Box(empty);
    for (int j = 0; j < n; ++j) {
      const T &prim = soup[j];
      centroids[j] = Centroid(prim);
      aabbs[j] = convertBox(prim.getAABB());
      sceneAABB.grow(aabbs[j]);
    }

    // Sort the bounding boxes along their axes
    for (int j = 0; j < n; ++j) 
      IDs[0][j] = IDs[1][j] = IDs[2][j] = j;
    std::sort(IDs[0], IDs[0]+n, CentroidSorter<0>(centroids));
    std::sort(IDs[1], IDs[1]+n, CentroidSorter<1>(centroids));
    std::sort(IDs[2], IDs[2]+n, CentroidSorter<2>(centroids));

    PF_MSG_V("BVH2: Injection time, " << getSeconds() - t);
    PF_ALIGNED_FREE(centroids);
  }

  /*! Grows up the bounding boxen to avoid precision problems */
  static const float aabbEps = 1e-6f;

  /*! The stack of nodes to process */
  struct Stack
  {
    /*! First and last index, ID and AAB of the currently processed node */
    struct Elem {
      INLINE Elem(void) {}
      INLINE Elem(int32 first, int32 last, uint32 index, const Box &aabb)
        : aabb(aabb), first(first), last(last), id(index) {}
      Box aabb;
      int32 first, last;
      uint32 id;
    };
    /*! Push a new element on the stack */
    INLINE void push(int a, int b, uint32 id, const Box &aabb) {
      assert(n < MAX_DEPTH-1);
      data[n++] = Stack::Elem(a, b, id, aabb);
    }
    /*! Pop an element from the stack */
    INLINE Elem pop(void) {
      assert(n > 0);
      return data[--n];
    }
    /*! Indicates if there is anything on the stack */
    INLINE bool isNotEmpty(void) const { return n != 0; }
    INLINE Stack(void) { n = 0; }

  private:
    enum { MAX_DEPTH = 64 }; //!< Maximum number of elements on the stack
    Elem data[MAX_DEPTH];    //!< Stack elements
    int n;                   //!< Current number of elements on the stack
  };

  /*! A partition of the current given sub-array */
  struct Partition
  {
    INLINE void init(int32 first_, int32 last_, uint32 d)
    {
      aabbs[ON_LEFT] = aabbs[ON_RIGHT] = Box(empty);
      first[ON_RIGHT] = first[ON_LEFT] = first_;
      last[ON_RIGHT] = last[ON_LEFT] = last_;
      axis = d;
      cost = FLT_MAX;
    }
    Box aabbs[2];
    int32 first[2], last[2];
    uint32 axis;
    float cost;
  };

  /*! Sweep left to right and find the best partition */
  template <uint32 axis>
  static void doSweep(BVH2Builder &c, Partition &part, int first, int last)
  {
    assert(first >= 0 && last < c.n);
    part.init(first, last, axis);

    // Compute sequence (from right to left) of the bounding boxes
    c.rlAABBs[c.IDs[axis][last]] = c.aabbs[c.IDs[axis][last]];
    for (int j  = last - 1; j >= first; --j) {
      c.rlAABBs[c.IDs[axis][j]] = c.aabbs[c.IDs[axis][j]];
      c.rlAABBs[c.IDs[axis][j]].grow(c.rlAABBs[c.IDs[axis][j + 1]]);
    }

    // Now, sweep from left to right and find the best partition (here we
    // ignore the traversal cost since we only compare strategies which DO
    // use a traversal -> the other strategy (intersecting all primitives
    // with no traversal is done afterward))
    Box aabb(empty);
    const uint32 primNum = last - first + 1;
    uint32 n = 1;
    part.cost = FLT_MAX;
    for (int j = first; j < last; ++j) {
      aabb.grow(c.aabbs[c.IDs[axis][j]]); 
      const float cost = halfArea(aabb) * float(n)
                       + halfArea(c.rlAABBs[c.IDs[axis][j + 1]]) * float(primNum - n);
      ++n;
      if (cost > part.cost) continue;
      part.cost = cost;
      part.last[ON_LEFT] = j;
      part.first[ON_RIGHT] = j + 1;
      part.aabbs[ON_LEFT] = aabb;
      part.aabbs[ON_RIGHT] = c.rlAABBs[c.IDs[axis][j + 1]];
    }

    // We want at most maxPrimNum
    if (primNum > c.options.maxPrimNum) return;

    // Test the last partition where all primitives are inside one node
    aabb.grow(c.aabbs[c.IDs[axis][last]]);
    const float harea = halfArea(aabb);
    const float cost = c.options.SAHIntersectionCost * harea * primNum;
    part.cost *= c.options.SAHIntersectionCost;
    part.cost += c.options.SAHTraversalCost * harea;
    if (cost <= part.cost) {
      part.cost = cost;
      part.last[ON_RIGHT]  = part.last[ON_LEFT]  = -1;
      part.first[ON_RIGHT] = part.first[ON_LEFT] = -1;
      part.aabbs[ON_RIGHT] = part.aabbs[ON_LEFT] = aabb;
    }
  }

  /*! Basically convert box into a node */
  INLINE void doSetNodeBBox(BVH2Node &node, const Box &box) {
    for (size_t j = 0; j < 3; ++j) node.pmin[j] = box.lower[j];
    for (size_t j = 0; j < 3; ++j) node.pmax[j] = box.upper[j];
  }

  /*! Store a node in the BVH2 */
  INLINE void doMakeNode(BVH2Builder &c, const Stack::Elem &data, uint32 axis) {
    c.root[data.id].setAxis(axis);
    doSetNodeBBox(c.root[data.id], data.aabb);
    c.root[data.id].setOffset(c.currID + 1);
    c.root[data.id].setAsNonLeaf();
  }

  /*! Store a leaf in the BVH2 */
  INLINE void doMakeLeaf(BVH2Builder &c, const Stack::Elem &data) {
    const uint32 primNum = data.last - data.first + 1;
    doSetNodeBBox(c.root[data.id], data.aabb);
    c.root[data.id].setPrimNum(primNum);
    c.root[data.id].setPrimID(c.primID.size());
    c.root[data.id].setAsLeaf();
    for (int j = data.first; j <= data.last; ++j)
      c.primID.push_back(c.IDs[0][j]);
  }

  void BVH2Builder::compile(void)
  {
    Stack stack;
    Stack::Elem node;

    stack.push(0, n - 1, 0, sceneAABB);
    while (stack.isNotEmpty()) {
      node = stack.pop();
      for (;;) {

        // We are done and we make a leaf
        const uint32 primNum = node.last - node.first + 1;
        if (primNum <= options.minPrimNum) {
          doMakeLeaf(*this, node);
          break;
        }

        // Find the best partition for this node
        Partition best, part;
        doSweep<0>(*this, best, node.first, node.last);
        doSweep<1>(*this, part, node.first, node.last);
        if (part.cost < best.cost) best = part;
        doSweep<2>(*this, part, node.first, node.last);
        if (part.cost < best.cost) best = part;

        // The best partition is actually *no* partition: we make a leaf
        if (best.first[ON_LEFT] == -1) {
          doMakeLeaf(*this, node);
          break;
        }

        // Register this node
        doMakeNode(*this, node, best.axis);

        // First, store the positions of the primitives
        const int d = best.axis;
        for (int j = best.first[ON_LEFT]; j <= best.last[ON_LEFT]; ++j)
          pos[IDs[d][j]] = ON_LEFT;
        for (int j = best.first[ON_RIGHT]; j <= best.last[ON_RIGHT]; ++j)
          pos[IDs[d][j]] = ON_RIGHT;

        // Then, for each axis, reorder the indices for the next step
        int leftNum, rightNum;
        for (int i = 0; i < otherAxisNum; ++i) {
          const int d0 = remapOtherAxis[best.axis + i];
          leftNum = 0, rightNum = 0;
          for (int j = node.first; j <= node.last; ++j)
            if (pos[IDs[d0][j]] == ON_LEFT)
              IDs[d0][node.first + leftNum++] = IDs[d0][j];
            else
              tmpIDs[rightNum++] = IDs[d0][j];
          for (int j = node.first + leftNum; j <= node.last; ++j)
            IDs[d0][j] = tmpIDs[j - leftNum - node.first];
        }

        // Now, prepare the stack data for the next step
        const int p0 = rightNum > leftNum ? ON_LEFT : ON_RIGHT;
        const int p1 = rightNum > leftNum ? ON_RIGHT : ON_LEFT;
        stack.push(best.first[p1], best.last[p1], currID+p1+1, best.aabbs[p1]);
        node.first = best.first[p0];
        node.last = best.last[p0];
        node.aabb = best.aabbs[p0];
        node.id = currID + p0 + 1;
        currID += 2;
      }
    }
  }

  const BVH2BuildOption defaultBVH2Options(2, 16, 1.f, 1.f);

  template <typename T>
  void buildBVH2(const T *t, uint32 primNum, BVH2<T> &tree, const BVH2BuildOption &option)
  {
    PF_MSG_V("BVH2: compiling BVH2");
    PF_MSG_V("BVH2: " << primNum << " primitives");
    BVH2Builder c;
    const double start = getSeconds();
    c.options = option;

    if (UNLIKELY(c.options.maxPrimNum < c.options.minPrimNum))
      FATAL("Bad BVH2 compilation parameters");

    c.injection<T>(t, primNum);
    c.compile();

    PF_MSG_V("BVH2: Compacting node array");
    tree.nodeNum = c.currID + 1;
    const size_t nodeSize = sizeof(BVH2Node) * tree.nodeNum;
    tree.node = (BVH2Node*) PF_ALIGNED_MALLOC(nodeSize, CACHE_LINE);
    std::memcpy(tree.node, c.root, nodeSize);
    PF_ALIGNED_FREE(c.root);
    PF_MSG_V("BVH2: " << tree.nodeNum << " nodes");
    uint32 leafNum = 0;
    for (size_t nodeID = 0; nodeID < tree.nodeNum; ++nodeID)
      if (tree.node[nodeID].isLeaf()) leafNum++;
    PF_MSG_V("BVH2: " << leafNum << " leaf nodes");
    PF_MSG_V("BVH2: " << tree.nodeNum - leafNum << " non-leaf nodes");
    PF_MSG_V("BVH2: " << double(primNum) / double(leafNum) << " primitives per leaf");

    PF_MSG_V("BVH2: Copying primitive soup");
    tree.primNum = primNum;
    tree.prim = PF_NEW_ARRAY(T, primNum);
    assert(tree.prim);
    std::memcpy(tree.prim, t, sizeof(T) * primNum);

    PF_MSG_V("BVH2: Copying primitive IDs");
    tree.primID = PF_NEW_ARRAY(uint32, tree.primNum);
    assert(tree.primID != NULL);
    std::memcpy(tree.primID, &c.primID[0], tree.primNum * sizeof(uint32));
    PF_MSG_V("BVH2: Time to build " << getSeconds() - start << " sec");
  }

  // Instantiation for RTTriangle
  template void buildBVH2<RTTriangle>(const RTTriangle*, uint32, BVH2<RTTriangle>&, const BVH2BuildOption&);
  template BVH2<RTTriangle>::BVH2(void);
  template BVH2<RTTriangle>::~BVH2(void);

} /* namespace pf */


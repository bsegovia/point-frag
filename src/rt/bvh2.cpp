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
#include <vector>

namespace pf
{
#if defined(__WIN32__)
  typedef BBox3f Box;

  INLINE Box convertBox(const BBox3f &from) { return from; }
#else
  typedef BBox<ssef> Box;

  /*! Computes half surface area of box. */
  INLINE float halfArea(const Box& box) {
    ssef d = size(box);
    ssef a = d*shuffle<1,2,0,3>(d);
    return extract<0>(reduce_add(a));
  }

  INLINE Box convertBox(const BBox3f &from) {
    Box to;
    for (size_t j = 0; j < 3; ++j) to.lower[j] = from.lower[j];
    for (size_t j = 0; j < 3; ++j) to.upper[j] = from.upper[j];
    return to;
  }
#endif /* __WIN32__ */

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
    int compile(void);

    std::vector<int> pos;
    std::vector<uint32> IDs[3];
    std::vector<uint32> primID;
    std::vector<uint32> tmpIDs;
    std::vector<Box> aabbs;
    std::vector<Box> rlAABBs;
    int n;
    int nodeNum;
    uint32 currID;
    Box sceneAABB;
    BVH2Node * RESTRICT root;
    BVH2BuildOption options;
  };

  /*! Sort the centroids along the given axis */
  template<uint32 axis>
  struct CentroidSorter : public std::binary_function<int, int, bool> {
    const std::vector<Centroid> &centroids;
    CentroidSorter(const std::vector<Centroid> &c) : centroids(c) {}
    INLINE int operator() (const uint32 a, const uint32 b) const {
      return centroids[a][axis] <  centroids[b][axis];
    }
  };

  /*! To perform the sorts in parallel */
  template <uint32 axis>
  class TaskCentroidSort : public Task
  {
  public:
    /*! Save the data in the task fields */
    TaskCentroidSort(const std::vector<Centroid> &centroids,
                     std::vector<uint32> &IDs) :
      Task("TaskCentroidSort"), centroids(centroids), IDs(IDs) {}

    /*! Increasing order sort for the given axis */
    virtual Task *run(void) {
      for (size_t j = 0; j < IDs.size(); ++j) IDs[j] = j;
      std::sort(IDs.begin(), IDs.end(), CentroidSorter<axis>(centroids));
      return NULL;
    }
  private:
    const std::vector<Centroid> &centroids; //!< Centroids of the primitives
    std::vector<uint32> &IDs;               //!< Array to sort
  };

  template <typename T>
  void BVH2Builder::injection(const T * const RESTRICT soup, uint32 primNum)
  {
    std::vector<Centroid> centroids;
    double t = getSeconds();

    // Allocate nodes and leaves for the BVH2
    nodeNum = 2 * primNum + 1;
    root = (BVH2Node *) PF_ALIGNED_MALLOC(nodeNum * sizeof(BVH2Node), 16);

    // Allocate the data for the compiler
    for (int i = 0; i < 3; ++i)
      IDs[i].resize(primNum);
    pos.resize(primNum);
    tmpIDs.resize(primNum);
    centroids.resize(primNum);
    aabbs.resize(primNum);
    rlAABBs.resize(primNum);
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
    std::sort(IDs[0].begin(), IDs[0].end(), CentroidSorter<0>(centroids));
    std::sort(IDs[1].begin(), IDs[1].end(), CentroidSorter<1>(centroids));
    std::sort(IDs[2].begin(), IDs[2].end(), CentroidSorter<2>(centroids));

    PF_MSG_V("BVH2: Injection time, " << getSeconds() - t);
  }

  BVH2Builder::BVH2Builder(void) : currID(0) { }
  BVH2Builder::~BVH2Builder(void) { }

  /*! Grows up the bounding boxen to avoid precision problems */
  static const float aabbEps = 1e-6f;

  /*! The stack of nodes to process */
  struct Stack
  {
    /*! First and last index, ID and AAB of the currently processed node */
    struct Elem {
      INLINE Elem(void) {}
      INLINE Elem(int first, int last, uint32 index, const Box &aabb)
        : first(first), last(last), id(index), aabb(aabb) {}
      int first, last;
      uint32 id;
      Box aabb;
    };
    /*! Push a new element on the stack */
    INLINE void push(int a, int b, uint32 id, const Box &aabb) {
      data[n++] = Stack::Elem(a, b, id, aabb);
    }
    /*! Pop an element from the stack */
    INLINE Elem pop(void) { return data[--n]; }
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
    INLINE Partition(void) {}
    INLINE Partition(int first_, int last_, uint32 d) :
      cost(FLT_MAX), axis(d) {
      aabbs[ON_LEFT] = aabbs[ON_RIGHT] = Box(empty);
      first[ON_RIGHT] = first[ON_LEFT] = first_;
      last[ON_RIGHT] = last[ON_LEFT] = last_;
    }
    Box aabbs[2];
    float cost;
    uint32 axis;
    int first[2], last[2];
  };

  /*! Sweep left to right and find the best partition */
  template <uint32 axis>
  static INLINE Partition doSweep(BVH2Builder &c, int first, int last)
  {
    // We return the best partition
    Partition part(first, last, axis);

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
    if (primNum > c.options.maxPrimNum) return part;

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

    return part;
  }

  /*! Basically convert box into a node */
  static INLINE void doSetNodeBBox(BVH2Node &node, const Box &box) {
    for (size_t j = 0; j < 3; ++j) node.pmin[j] = box.lower[j];
    for (size_t j = 0; j < 3; ++j) node.pmax[j] = box.upper[j];
  }

  /*! Store a node in the BVH2 */
  static INLINE void doMakeNode(BVH2Builder &c, const Stack::Elem &data, uint32 axis) {
    c.root[data.id].setAxis(axis);
    doSetNodeBBox(c.root[data.id], data.aabb);
    c.root[data.id].setOffset(c.currID + 1);
    c.root[data.id].setAsNonLeaf();
  }

  /*! Store a leaf in the BVH2 */
  static INLINE void doMakeLeaf(BVH2Builder &c, const Stack::Elem &data) {
    const uint32 primNum = data.last - data.first + 1;
    doSetNodeBBox(c.root[data.id], data.aabb);
    c.root[data.id].setPrimNum(primNum);
    c.root[data.id].setPrimID(c.primID.size());
    c.root[data.id].setAsLeaf();
    for (int j = data.first; j <= data.last; ++j)
      c.primID.push_back(c.IDs[0][j]);
  }

  /*! Grow the bounding boxen with an epsilon */
  static INLINE void doGrowBoxes(BVH2Builder &c) {
    const int aabbNum = 2 * c.n - 1;
    for (int i = 0; i < aabbNum; ++i) {
      vec3f pmin = c.root[i].getMin();
      vec3f pmax = c.root[i].getMax();
      for (int j = 0; j < 3; ++j) {
        const float d = abs(pmax[j] - pmin[j]);
        pmin[j] -= aabbEps * d;
        pmax[j] += aabbEps * d;
      }
      c.root[i].setMin(pmin);
      c.root[i].setMax(pmax);
    }
  }

  int BVH2Builder::compile(void)
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
        Partition best = doSweep<0>(*this, node.first, node.last);
        Partition part = doSweep<1>(*this, node.first, node.last);
        if (part.cost < best.cost) best = part;
        part = doSweep<2>(*this, node.first, node.last);
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

    doGrowBoxes(*this);
    return 0;
  }

  const BVH2BuildOption defaultBVH2Options(2, 16, 1.f, 1.f);

  template <typename T>
  void buildBVH2(const T *t, uint32 primNum, BVH2<T> &tree, const BVH2BuildOption &option)
  {
    PF_MSG_V("BVH2: compiling BVH2");
    PF_MSG_V("BVH2: " << primNum << " primitives");
    Ref<BVH2Builder> c = PF_NEW(BVH2Builder);
    const double start = getSeconds();
    c->options = option;

    if (UNLIKELY(c->options.maxPrimNum < c->options.minPrimNum))
      FATAL("Bad BVH2 compilation parameters");

    c->injection<T>(t, primNum);
    c->compile();

    PF_MSG_V("BVH2: Compacting node array");
    tree.nodeNum = c->currID + 1;
    const size_t nodeSize = sizeof(BVH2Node) * tree.nodeNum;
    tree.node = (BVH2Node*) PF_ALIGNED_MALLOC(nodeSize, CACHE_LINE);
    std::memcpy(tree.node, c->root, nodeSize);
    PF_ALIGNED_FREE(c->root);
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
    std::memcpy(tree.primID, &c->primID[0], tree.primNum * sizeof(uint32));
    PF_MSG_V("BVH2: Time to build " << getSeconds() - start << " sec");
  }

  // Instantiation for RTTriangle
  template void buildBVH2<RTTriangle>
    (const RTTriangle*, uint32, BVH2<RTTriangle>&, const BVH2BuildOption&);
  template BVH2<RTTriangle>::BVH2(void);
  template BVH2<RTTriangle>::~BVH2(void);

} /* namespace pf */


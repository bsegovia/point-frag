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

#include "bvh.hpp"
#include "rt_triangle.hpp"

#include "math/bbox.hpp"
#include "sys/logging.hpp"
#include "sys/platform.hpp"

#include <algorithm>
#include <functional>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>

namespace pf
{
  /* Just the barycenter of a triangle */
  struct Centroid : public vec3f {
    Centroid() {}
    Centroid(const RTTriangle &t) :
      vec3f(t.v[0].x + t.v[1].x + t.v[2].x,
            t.v[0].y + t.v[1].y + t.v[2].y,
            t.v[0].z + t.v[1].z + t.v[2].z) {}
  };

  /*! To sort primitives */
  enum { ON_LEFT = 0, ON_RIGHT = 1 };

  /*! Maximum and minimum number of primitives in a leaf */
  enum { minPrimNumPerLeaf = 2 };
  enum { maxPrimNumPerLeaf = 14 };

  /*! SAH cost for traversal and intersection */
  static const float SAHIntersectionCost = 1.f;
  static const float SAHTraversalCost = 1.f;

  /*! Used when sorting along each axis */
  static const int remapOtherAxis[4] = {1, 2, 0, 1};
  enum {otherAxisNum = 2};

  /*! Options to compile the BVH */
  struct BVHCompilerOption {
    uint32 minPrimNum;
    uint32 maxPrimNum;
    float SAHIntersectionCost;
    float SAHTraversalCost;
  };

  /*! n.log(n) compiler with bounding box sweeping and SAH */
  struct BVHCompiler : public RefCount
  {
    BVHCompiler(void);
    ~BVHCompiler(void);
    /*! Compute primitives centroids and sort then for each axis */
    template <typename T>
    int injection(const T * const RESTRICT soup, uint32 primNum);
    /*! Build the hierarchy itself */
    int compile(void);

    std::vector<int> pos;
    std::vector<uint32> IDs[3];
    std::vector<uint32> primID;
    std::vector<uint32> tmpIDs;
    std::vector<BBox3f> aabbs;
    std::vector<BBox3f> rlAABBs;
    int n;
    int nodeNum;
    uint32 currID;
    BBox3f sceneAABB;
    BVHNode * RESTRICT root;
    BVHCompilerOption options;
  };

  /*! Sort the centroids along the given axis */
  template<uint32 axis>
  struct Sorter : public std::binary_function<int, int, bool> {
    const std::vector<Centroid> &centroids;
    Sorter(const std::vector<Centroid> &c) : centroids(c) {}
    INLINE int operator() (const uint32 a, const uint32 b) const  {
      return centroids[a][axis] <  centroids[b][axis];
    }
  };

  template <typename T>
  int BVHCompiler::injection(const T * const RESTRICT soup, uint32 primNum)
  {
    std::vector<Centroid> centroids;

    // Allocate nodes and leaves for the BVH
    root = (BVHNode *) new BVHNode [2 * primNum + 1];
    nodeNum = 2 * primNum + 1;
    if (root == 0) return -1;

    // Allocate the data for the compiler
    for(int i = 0; i < 3; ++i) {
      IDs[i].resize(primNum);
      if(IDs[i].size() < primNum) return -1;
    }
    pos.resize(primNum);
    tmpIDs.resize(primNum);
    centroids.resize(primNum);
    aabbs.resize(primNum);
    rlAABBs.resize(primNum);
    n = primNum;

    // Compute centroids and bounding boxes
    sceneAABB = BBox3f(empty);
    for(int j = 0; j < n; ++j) {
      const T &pri = soup[j];
      centroids[j] = Centroid(pri);
      aabbs[j] = pri.getAABB();
      sceneAABB.grow(aabbs[j]);
    }

    // Sort the centroids
    for(int i = 0; i < 3; ++i)
      for(int j = 0; j < n; ++j)
        IDs[i][j] = j;
    std::sort(IDs[0].begin(), IDs[0].end(), Sorter<0>(centroids));
    std::sort(IDs[1].begin(), IDs[1].end(), Sorter<1>(centroids));
    std::sort(IDs[2].begin(), IDs[2].end(), Sorter<2>(centroids));

    return n;
  }

  /*! Explicit instantiation for RTTriangle */
  template int BVHCompiler::injection<RTTriangle> (const RTTriangle * const RESTRICT, uint32);

  BVHCompiler::BVHCompiler() : currID(0) { }
  BVHCompiler::~BVHCompiler() { }

  /*! Grows up the bounding boxen to avoid precision problems */
  static const float aabbEps = 1e-6f;

  /*! The stack of nodes to process */
  struct Stack
  {
    /*! First and last index, ID and AAB of the currently processed node */
    struct Elem {
      int first, last;
      uint32 id;
      BBox3f aabb;
      INLINE Elem(void) {}
      INLINE Elem(int f, int l, uint32 index, const BBox3f &paabb)
        : first(f), last(l), id(index), aabb(paabb) {}
    };
    /*! Push a new element on the stack */
    INLINE void push(int a, int b, uint32 id, const BBox3f &aabb) {
      data[n++] = Stack::Elem(a, b, id, aabb);
    }
    /*! Pop an element from the stack */
    INLINE Elem pop(void) { return data[--n]; }
    /*! Indicates if there is anything on the stack */
    INLINE bool isNotEmpty(void) const { return n != 0; }
    INLINE Stack(void) { n = 0; }

  private:
    enum { MAX_SZ = 64 }; //!< Maximum number of elements on the stack
    Elem data[MAX_SZ];    //!< Stack elements
    int n;                //!< Current number of elements on the stack
  };

  /*! A partition of the current given sub-array */
  struct Partition
  {
    Partition() {}
    Partition(int f, int l, uint32 d) :
      cost(FLT_MAX), axis(d) {
      aabbs[ON_LEFT] = aabbs[ON_RIGHT] = BBox3f(empty);
      first[ON_RIGHT] = first[ON_LEFT] = f;
      last[ON_RIGHT] = last[ON_LEFT] = l;
    }
    BBox3f aabbs[2];
    float cost;
    uint32 axis;
    int first[2], last[2];
  };

  /*! Sweep left to right and find the best partition */
  template <uint32 axis>
  static INLINE Partition doSweep(BVHCompiler &c, int first, int last)
  {
    // We return the best partition
    Partition part(first, last, axis);

    // Compute sequence (from right to left) of the bounding boxes
    c.rlAABBs[c.IDs[axis][last]] = c.aabbs[c.IDs[axis][last]];
    for(int j  = last - 1; j >= first; --j) {
      c.rlAABBs[c.IDs[axis][j]] = c.aabbs[c.IDs[axis][j]];
      c.rlAABBs[c.IDs[axis][j]].grow(c.rlAABBs[c.IDs[axis][j + 1]]);
    }

    // Now, sweep from left to right and find the best partition (here we
    // ignore the traversal cost since we only compare strategies which DO
    // use a traversal -> the other strategy (intersecting all primitives
    // with no traversal is done afterward))
    BBox3f aabb(empty);
    const uint32 primNum = (float) (last - first) + 1.f;
    uint32 n = 1;
    part.cost = FLT_MAX;
    for(int j = first; j < last; ++j) {
      aabb.grow(c.aabbs[c.IDs[axis][j]]);
      const float cost = area(aabb) * float(n)
        + area(c.rlAABBs[c.IDs[axis][j + 1]]) * float(primNum - n);
      ++n;
      if(cost > part.cost) continue;
      part.cost = cost;
      part.last[ON_LEFT] = j;
      part.first[ON_RIGHT] = j + 1;
      part.aabbs[ON_LEFT] = aabb;
      part.aabbs[ON_RIGHT] = c.rlAABBs[c.IDs[axis][j + 1]];
    }

    // We want at most maxPrimNum
    if(primNum > c.options.maxPrimNum) return part;

    // Test the last partition where all primitives are inside one node
    aabb.grow(c.aabbs[c.IDs[axis][last]]);
    const float barea = area(aabb);
    const float cost = c.options.SAHIntersectionCost * barea * primNum;
    part.cost *= c.options.SAHIntersectionCost;
    part.cost += c.options.SAHTraversalCost * barea;
    if(cost <= part.cost) {
      part.cost = cost;
      part.last[ON_LEFT] = -1;
      part.last[ON_RIGHT] = -1;
      part.first[ON_LEFT] = -1;
      part.first[ON_RIGHT] = -1;
      part.aabbs[ON_LEFT] = aabb;
      part.aabbs[ON_RIGHT] = aabb;
    }

    return part;
  }

  /*! Store a node in the BVH */
  static INLINE void doMakeNode(BVHCompiler &c, const Stack::Elem &data, uint32 axis) {
    c.root[data.id].setAxis(axis);
    c.root[data.id].setMin(data.aabb.lower);
    c.root[data.id].setMax(data.aabb.upper);
    c.root[data.id].setOffset(c.currID + 1);
    c.root[data.id].setAsNonLeaf();
  }

  /*! Store a leaf in the BVH */
  static INLINE void doMakeLeaf(BVHCompiler &c, const Stack::Elem &data) {
    const uint32 primNum = data.last - data.first + 1;
    c.root[data.id].setMin(data.aabb.lower);
    c.root[data.id].setMax(data.aabb.upper);
    c.root[data.id].setPrimNum(primNum);
    c.root[data.id].setPrimID(c.primID.size());
    c.root[data.id].setAsLeaf();
    for(int j = data.first; j <= data.last; ++j)
      c.primID.push_back(c.IDs[0][j]);
  }

  /*! Grow the bounding boxen with an epsilon */
  static INLINE void doGrowBBox3fs(BVHCompiler &c) {
    const int aabb_n = 2 * c.n - 1;
    for(int i = 0; i < aabb_n; ++i) {
      vec3f pmin = c.root[i].getMin();
      vec3f pmax = c.root[i].getMax();
      for(int j = 0; j < 3; ++j) {
        pmin[j] *= pmin[j] < 0.f ? 1.f + aabbEps : 1.f - aabbEps;
        pmax[j] *= pmax[j] > 0.f ? 1.f + aabbEps : 1.f - aabbEps;
      }
      c.root[i].setMin(pmin);
      c.root[i].setMax(pmax);
    }
  }

  int BVHCompiler::compile(void) {
    Stack stack;
    Stack::Elem node;

    stack.push(0, n - 1, 0, sceneAABB);
    while(stack.isNotEmpty()) {
      node = stack.pop();
      while(1) {

        // We are done and we make a leaf
        const uint32 primNum = node.last - node.first + 1;
        if(primNum <= options.minPrimNum) {
          doMakeLeaf(*this, node);
          break;
        }

        // Find the best partition for this node
        Partition best = doSweep<0>(*this, node.first, node.last);
        Partition part = doSweep<1>(*this, node.first, node.last);
        if(part.cost < best.cost) best = part;
        part = doSweep<2>(*this, node.first, node.last);
        if(part.cost < best.cost) best = part;

        // The best partition is actually *no* partition: we make a leaf
        if(part.first[ON_LEFT] == -1) {
          doMakeLeaf(*this, node);
          break;
        }

        // Register this node
        doMakeNode(*this, node, best.axis);

        // First, store the positions of the primitives
        const int d = best.axis;
        for(int j = best.first[ON_LEFT]; j <= best.last[ON_LEFT]; ++j)
          pos[IDs[d][j]] = ON_LEFT;
        for(int j = best.first[ON_RIGHT]; j <= best.last[ON_RIGHT]; ++j)
          pos[IDs[d][j]] = ON_RIGHT;

        // Then, for each axis, reorder the indices for the next step
        int left_n, right_n;
        for(int i = 0; i < otherAxisNum; ++i) {
          const int d0 = remapOtherAxis[best.axis + i];
          left_n = 0, right_n = 0;
          for(int j = node.first; j <= node.last; ++j)
            if(pos[IDs[d0][j]] == ON_LEFT)
              IDs[d0][node.first + left_n++] = IDs[d0][j];
            else
              tmpIDs[right_n++] = IDs[d0][j];
          for(int j = node.first + left_n; j <= node.last; ++j)
            IDs[d0][j] = tmpIDs[j - left_n - node.first];
        }

        // Now, prepare the stack data for the next step
        const int p0 = right_n > left_n ? ON_LEFT : ON_RIGHT;
        const int p1 = right_n > left_n ? ON_RIGHT : ON_LEFT;
        stack.push(best.first[p1], best.last[p1], currID+p1+1, best.aabbs[p1]);
        node.first = best.first[p0];
        node.last = best.last[p0];
        node.aabb = best.aabbs[p0];
        node.id = currID + p0 + 1;
        currID += 2;
      }
    }

    doGrowBBox3fs(*this);
    return 0;
  }

  /*! Compile a BVH */
  template <typename T>
  bool compile(const T *t, uint32 primNum, BVH<T> &tree, uint32 minPrimNum, uint32 maxPrimNum)
  {
    PF_MSG_V("BVH: compiling BVH");
    PF_MSG_V("BVH: number of primitives: " << primNum);
    Ref<BVHCompiler> c = new BVHCompiler;
    const double start = getSeconds();
    c->options.minPrimNum = std::max(minPrimNum, (uint32) minPrimNumPerLeaf);
    c->options.maxPrimNum = std::min(maxPrimNum, (uint32) maxPrimNumPerLeaf);
    c->options.SAHIntersectionCost = SAHIntersectionCost;
    c->options.SAHTraversalCost = SAHTraversalCost;

    if (UNLIKELY(c->options.maxPrimNum < c->options.minPrimNum))
      FATAL("Bad BVH compilation parameters");

    if (c->injection<T>(t, primNum) < 0) return false;
    c->compile();

    tree.node = c->root;
    tree.nodeNum = c->currID + 1;
    PF_MSG_V("BVH: nodeNum " << tree.nodeNum);
    PF_MSG_V("BVH: Copying primitive soup\n");
    tree.primNum = primNum;
    tree.prim = new T[primNum];
    assert(tree.prim);
    std::memcpy(tree.prim, t, sizeof(T) * primNum);

    PF_MSG_V("BVH: Copying primitive IDs\n");
    tree.primID = new uint32_t[tree.primNum];
    assert(tree.primID != NULL);
    std::memcpy(tree.primID, &c->primID[0], tree.primNum * sizeof(uint32_t));
    PF_MSG_V("BVH: Time to build " << getSeconds() - start << " sec");
    return true;
  }

  /*! Instantiation for RTTriangle */
  template bool compile<RTTriangle> (const RTTriangle*, uint32, BVH<RTTriangle>&, uint32, uint32);

  template <typename T>
  BVH<T>::BVH(void) : node(NULL), prim(NULL), primID(NULL), nodeNum(0), primNum(0) {}

  template <typename T>
  BVH<T>::~BVH(void) {
    SAFE_DELETE_ARRAY(this->node);
    SAFE_DELETE_ARRAY(this->primID);
    SAFE_DELETE_ARRAY(this->prim);
  }

} /* namespace pf */


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

#include "ray.hpp"
#include "bvh.hpp"
#include "rt_triangle.hpp"

namespace pf
{
  /*! Generic ray / primitive intersection */
  template <typename T>
  static INLINE void PrimIntersect(const T &prim, uint32_t id, const Ray &ray, Hit &hit);

  /*! Node / ray intersection */
  static INLINE bool
  NodeIntersect(const BVH2Node &node, const Ray &ray, float t)
  {
    const vec3f l1 = (node.pmin - ray.org) * ray.rdir;
    const vec3f l2 = (node.pmax - ray.org) * ray.rdir;
    const vec3f nearVec = min(l1,l2);
    const vec3f farVec = max(l1,l2);
    const float near = max(max(nearVec[0], nearVec[1]), nearVec[2]);
    const float far = min(min(farVec[0], farVec[1]), farVec[2]);
    return (far >= near) & (far >= 0.f) & (near < t);
  }

  /*! To store call stack while traversing the BVH */
  struct RayStack
  {
    /*! Stack is empty */
    INLINE RayStack(void) : idx(0) {}
    /*! Remove one element from the stack */
    INLINE bool pop(void) { return --idx >= 0; }
    /*! Maximum stack depth */
    enum { MAX_DEPTH = 128 };
    /*! All the pushed nodes */
    const BVH2Node * RESTRICT elem[MAX_DEPTH];
    /*! Current size of the stack */
    int32 idx;
  };

  /*! Generic ray / BVH intersection algorithm */
  template <typename T>
  void trace(const BVH2<T> &bvh, const Ray &ray, Hit &hit)
  {
    RayStack stack;
    const uint32 signs[3] = {
      ray.dir.x < 0.f ? 1 : 0,
      ray.dir.y < 0.f ? 1 : 0,
      ray.dir.z < 0.f ? 1 : 0
    };

    stack.elem[0] = bvh.node;
    ++stack.idx;

  popLabel:
    // Recurse until we find a leaf
    while (LIKELY(stack.pop())) {
      const BVH2Node * RESTRICT node = stack.elem[stack.idx];
      while (LIKELY(!node->isLeaf())) {
        if (!NodeIntersect(*node, ray, hit.t)) goto popLabel;
        const uint32 offset = node->getOffset();
        const uint32 first = signs[node->getAxis()];
        const uint32 next = first ^ 1;
        const BVH2Node * RESTRICT left = &bvh.node[offset + next];
        const uint32 idx = stack.idx++;
        stack.elem[idx] = left;
        node = &bvh.node[node->getOffset() + first];
      }

      // Intersect BVH leaf
      if (!NodeIntersect(*node, ray, hit.t)) goto popLabel;
      const uint32 first = node->getPrimID();
      const uint32 triNum = node->getPrimNum();
      for(uint32 i = 0; i < triNum; ++i) {
        const uint32 primID = bvh.primID[first + i];
        PrimIntersect(bvh.prim[primID], primID, ray, hit);
      }
    }
  }

  /*! Single ray / Pluecker triangle intersection */
  template <>
  INLINE void PrimIntersect<RTTriangle>(const RTTriangle &tri, uint32_t id, const Ray &ray, Hit &hit)
  {
    const vec3f a = tri.v[0];
    const vec3f b = tri.v[1];
    const vec3f c = tri.v[2];
    const vec3f d0 = a - ray.org;
    const vec3f n = cross(c - a, b - a);
    const float num = dot(n, d0);
    const vec3f d1 = b - ray.org;
    const vec3f d2 = c - ray.org;
    const vec3f v0 = cross(d1, d2);
    const vec3f v1 = cross(d0, d1);
    const vec3f v2 = cross(d2, d0);
    const float u = dot(v0, ray.dir);
    const float v = dot(v1, ray.dir);
    const float w = dot(v2, ray.dir);
#if USE_BACKFACE_CULLING
    bool sameSign = ((u < 0) & (v < 0) & (w < 0));
#else
    bool sameSign = (((u > 0) & (v > 0) & (w > 0)) | ((u < 0) & (v < 0) & (w < 0)));
#endif
    if (!sameSign) return;

    const float n0 = dot(n, ray.dir);
    const float t = num / n0;
    if ((t > hit.t) || (t < 0.f)) return;
    hit.t = t;
    hit.u = u;
    hit.v = v;
    hit.id0 = id;
  }

  /*! Explicit instantiation for BVH2s of RTTriangle */
  template void trace(const BVH2<RTTriangle>&, const Ray&, Hit&);

} /* namespace BKY */


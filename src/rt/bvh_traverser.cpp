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
#include "bvh2_node.hpp"
#include "ray.hpp"
#include "ray_packet.hpp"
#include "rt_triangle.hpp"

namespace pf
{
  ///////////////////////////////////////////////////////////////////////////
  /// Single Ray Routines
  ///////////////////////////////////////////////////////////////////////////

  /*! Generic ray / primitive intersection */
  template <typename T>
  static INLINE void PrimIntersect(const T&, uint32_t, const Ray&, Hit&);

  /*! Node AABB / ray intersection */
  static INLINE bool AABBIntersect(const BVH2Node &node, const Ray &ray, float t)
  {
    const vec3f l1 = (node.pmin - ray.org) * ray.rdir;
    const vec3f l2 = (node.pmax - ray.org) * ray.rdir;
    const vec3f nearVec = min(l1,l2);
    const vec3f farVec = max(l1,l2);
    const float near = max(max(nearVec[0], nearVec[1]), nearVec[2]);
    const float far = min(min(farVec[0], farVec[1]), farVec[2]);
    return (far >= near) & (far >= 0.f) & (near < t);
  }

  /*! Generic ray / leaf intersection */
  template <typename T>
  static INLINE void LeafIntersect
    (const BVH2<T> &bvh, const BVH2Node &node, const Ray &ray, Hit &hit)
  {
    if (AABBIntersect(node, ray, hit.t)) {
      const uint32 firstPrim = node.getPrimID();
      const uint32 primNum = node.getPrimNum();
      for(uint32 i = 0; i < primNum; ++i) {
        const uint32 primID = bvh.primID[firstPrim + i];
        PrimIntersect(bvh.prim[primID], primID, ray, hit);
      }
    }
  }

  /*! Single ray / Pluecker triangle intersection. For single rays, it is
   *  actually bad (see the remark done about Pluecker intersector for
   *  packets). I should clearly replace it by something else
   */
  template <>
  INLINE void PrimIntersect<RTTriangle>
    (const RTTriangle &tri, uint32_t id, const Ray &ray, Hit &hit)
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
    const bool aperture = ((u>0)&(v>0)&(w>0)) | ((u<0)&(v<0)&(w<0));
    if (!aperture) return;
    const float n0 = dot(n, ray.dir);
    const float t = num / n0;
    if ((t > hit.t) || (t < 0.f)) return;
    hit.t = t;
    hit.u = u;
    hit.v = v;
    hit.id0 = id;
  }

  /*! To store call stack while traversing the BVH */
  struct RayStack
  {
    /*! Stack is empty */
    INLINE RayStack(void) : top(0) {}
    /*! Remove one element from the stack */
    INLINE bool pop(void) { return --top >= 0; }
    /*! Push a new element */
    INLINE void push(const BVH2Node * RESTRICT node) { elem[top++] = node; }
    enum { MAX_DEPTH = 128 };                  //!< Maximum stack depth 
    const BVH2Node * RESTRICT elem[MAX_DEPTH]; //!< All the pushed nodes
    int32 top;                                 //!< Current size of the stack
  };

  template <typename T>
  void BVH2<T>::traverse(const Ray &ray, Hit &hit) const
  {
    RayStack stack;
    const uint32 signArray[3] = {
      ray.dir.x<0.f ? 1u : 0u,
      ray.dir.y<0.f ? 1u : 0u,
      ray.dir.z<0.f ? 1u : 0u
    };
    stack.push(this->node);

  popNode:
    // Recurse until we find a leaf
    while (LIKELY(stack.pop())) {
      const BVH2Node * RESTRICT node = stack.elem[stack.top];
      while (LIKELY(!node->isLeaf())) {
        if (!AABBIntersect(*node, ray, hit.t)) goto popNode;
        const uint32 offset = node->getOffset();
        const uint32 first = signArray[node->getAxis()];
        const uint32 second = first ^ 1;
        stack.push(this->node + offset + second);
        node = &this->node[offset + first];
      }

      // Intersect BVH leaf and its primitives
      LeafIntersect(*this, *node, ray, hit);
    }
  }

  template <typename T>
  bool BVH2<T>::occluded(const Ray &ray) const {
    NOT_IMPLEMENTED;
    return false;
  }

  /*! Explicit instantiation for BVH2s of RTTriangle */
  template void BVH2<RTTriangle>::traverse(const Ray&, Hit&) const;
  template bool BVH2<RTTriangle>::occluded(const Ray&) const;

  ///////////////////////////////////////////////////////////////////////////
  /// Ray Packet Routines
  ///////////////////////////////////////////////////////////////////////////

  /*! Generic ray / primitive intersection */
  template <typename T>
  static INLINE void PrimIntersect
    (const T &, uint32_t, const RayPacket&, const uint32*, uint32, PacketHit&);

  /*! Kay-Kajiya AABB intersection */
  static INLINE void slab
    (const sse3f &rdir, const sse3f &minOrg, const sse3f &maxOrg, ssef &near, ssef &far)
  {
    ssef l1 = minOrg.x * rdir.x;
    ssef l2 = maxOrg.x * rdir.x;
    near = min(l1,l2);
    far  = max(l1,l2);
    l1   = minOrg.y * rdir.y;
    l2   = maxOrg.y * rdir.y;
    near = max(min(l1,l2), near);
    far  = min(max(l1,l2), far);
    l1   = minOrg.z * rdir.z;
    l2   = maxOrg.z * rdir.z;
    near = max(min(l1,l2), near);
    far  = min(max(l1,l2), far);
  }

  /*! Kay-Kajiya AABB with interval arithmetic */
  static INLINE bool slabIA(ssef minOrg, ssef maxOrg, sseb sign, ssef rcpMin, ssef rcpMax)
  {
    const ssef minusMin = -minOrg;
    const ssef minusMax = -maxOrg;
    const ssef aMin = select(sign, minusMax, minOrg);
    const ssef aMax = select(sign, minusMin, maxOrg);
    const ssef pMin = aMin * rcpMin;
    const ssef pMax = aMax * rcpMax;
    const ssef aMinMM = min(pMin, pMax);
    const ssef aMaxMM = max(pMin, pMax);
    const ssef near = aMinMM;
    const ssef far = reduce_min(aMaxMM);
    const sseb ia = (near > far) | sseb(far);
    return movemask(ia) & 0x7;
  }

  /*! AABB (non leaf node) / packet intersection */
  static INLINE bool AABBIntersect
    (const BVH2Node &node, const RayPacket &pckt, const PacketHit &hit, uint32 &first)
  {
//    return true;
    // Avoid issues with w unused channel
    const ssef lower = ssef(&node.pmin.x).xyzz();
    const ssef upper = ssef(&node.pmax.x).xyzz();
    const ssef tmpMin = lower - pckt.iaMaxOrg;
    const ssef tmpMax = upper - pckt.iaMinOrg;

    // Try fast exit if the packet supports interval arithmetic
    if (pckt.properties & RAY_PACKET_IA)
      if (slabIA(tmpMin, tmpMax, pckt.iasign, pckt.iaMinrDir, pckt.iaMaxrDir))
        return false;

    // Fast path with common origin packets
    if (pckt.properties & RAY_PACKET_CO) {
      const sse3f dmin(tmpMin.xxxx(), tmpMin.yyyy(), tmpMin.zzzz());
      const sse3f dmax(tmpMax.xxxx(), tmpMax.yyyy(), tmpMax.zzzz());
      for (uint32 i = first; i < pckt.packetNum; ++i) {
        ssef near, far;
        slab(pckt.rdir[i], dmin, dmax, near, far);
        const sseb test = (far >= near) & (far > 0.f) & (near < hit.t[i]);
        if (movemask(test)) {
          first = i;
          return true;
        }
      }
    } else {
      const sse3f pmin(node.pmin.x, node.pmin.y, node.pmin.z);
      const sse3f pmax(node.pmax.x, node.pmax.y, node.pmax.z);
      for (uint32 i = first; i < pckt.packetNum; ++i) {
        const sse3f dmin = pmin - pckt.org[i];
        const sse3f dmax = pmax - pckt.org[i];
        ssef near, far;
        slab(pckt.rdir[i], dmin, dmax, near, far);
        const sseb test = (far >= near) & (far > 0.f) & (near < hit.t[i]);
        if (movemask(test)) {
          first = i;
          return true;
        }
      }
    }
    return false;
  }

  /*! AABB (leaf node) / packet intersection. Track all active rays */
  static INLINE bool AABBIntersect
    (const BVH2Node &node, const RayPacket &pckt, const PacketHit &hit,
     uint32 first, uint32 *active, uint32 &activeNum)
  {
    // Avoid issues with w unused channel
    const ssef lower = ssef(&node.pmin.x).xyzz();
    const ssef upper = ssef(&node.pmax.x).xyzz();
    const ssef tmpMin = lower - pckt.iaMaxOrg;
    const ssef tmpMax = upper - pckt.iaMinOrg;

    // Use interval arithmetic test if supported
    if (pckt.properties & RAY_PACKET_IA)
      if (slabIA(tmpMin, tmpMax, pckt.iasign, pckt.iaMinrDir, pckt.iaMaxrDir))
        return false;

    // Fast path with common origin packets
    bool isIntersected = false;
    activeNum = 0;
    if (pckt.properties & RAY_PACKET_CO) {
      const sse3f dmin(tmpMin.xxxx(), tmpMin.yyyy(), tmpMin.zzzz());
      const sse3f dmax(tmpMax.xxxx(), tmpMax.yyyy(), tmpMax.zzzz());
      for(uint32 i = first; i < pckt.packetNum; ++i) {
        ssef near, far;
        slab(pckt.rdir[i], dmin, dmax, near, far);
        const sseb test = (far >= near) & (far > 0.f) & (near < hit.t[i]);
        if (movemask(test)) {
          active[activeNum++] = i;
          isIntersected = true;
        }
      }
    } else {
      const sse3f pmin(node.pmin.x, node.pmin.y, node.pmin.z);
      const sse3f pmax(node.pmax.x, node.pmax.y, node.pmax.z);
      for (uint32 i = first; i < pckt.packetNum; ++i) {
        const sse3f dmin = pmin - pckt.org[i];
        const sse3f dmax = pmax - pckt.org[i];
        ssef near, far;
        slab(pckt.rdir[i], dmin, dmax, near, far);
        const sseb test = (far >= near) & (far > 0.f) & (near < hit.t[i]);
        if (movemask(test)) {
          active[activeNum++] = i;
          isIntersected = true;
        }
      }
    }
    return isIntersected;
  }

  /*! Generic Packet / Leaf intersection */
  template <typename T>
  static INLINE void LeafIntersect
    (const BVH2<T> &bvh, const BVH2Node &node, const RayPacket &pckt, uint32 first, PacketHit &hit)
  {
    uint32 active[RayPacket::packetNum];
    uint32 activeNum;
    if (AABBIntersect(node, pckt, hit, first, active, activeNum)) {
      const uint32 firstPrim = node.getPrimID();
      const uint32 primNum = node.getPrimNum();
      for(uint32 i = 0; i < primNum; ++i) {
        const uint32 primID = bvh.primID[firstPrim + i];
        PrimIntersect(bvh.prim[primID], primID, pckt, active, activeNum, hit);
      }
    }
  }

  static INLINE ssef crossZXY(const ssef &a, const ssef &b) {
    return a*b.yzxx() - a.yzxx()*b;
  }
  static INLINE ssef dotZXY(const ssef &a, const ssef &b) {
    return a.xxxx()*b.zzzz() + a.yyyy()*b.xxxx() + a.zzzz()*b.yyyy();
  }
  static INLINE ssef dotZXY(const sse3f &a, const sse3f &b) {
    return a.x * b.z + a.y * b.x + a.z * b.y;
  }

  /*! Pluecker intersection
   *  Pros:
   *  - super fast with common origin rays. You precompute the planes and then
   *    it is mostly three dot products
   *  - aperture test is done first and this is really better than doing the
   *    depth test first
   *  Cons:
   *  - Numerically bad when triangles are far away
   *  - Expensive when origin is not shared
   *  TODO Take a better intersector for non-common-origin ray packets
   * */
  template <>
  INLINE void PrimIntersect<RTTriangle>
    (const RTTriangle &tri, uint32_t id, const RayPacket &pckt,
     const uint32 *active, uint32 activeNum, PacketHit &hit)
  {
    if (pckt.properties | RAY_PACKET_CO) {
      const ssef a(&tri.v[0].x);
      const ssef b(&tri.v[1].x);
      const ssef c(&tri.v[2].x);
      const ssef d0 = a - pckt.iaMinOrg;
      const ssef ca = c - a;
      const ssef ba = b - a;
      const ssef cb = c - b;
      const ssef sn = crossZXY(ca, ba);
      const ssef snum = dotZXY(sn, d0);
      const ssef d1 = b - pckt.iaMinOrg;
      const ssef sv0 = crossZXY(d1, cb);
      const ssef sv1 = crossZXY(d0, ba);
      const ssef sv2 = crossZXY(ca, d0);
      const sse3f v0(sv0.xxxx(), sv0.yyyy(), sv0.zzzz());
      const sse3f v1(sv1.xxxx(), sv1.yyyy(), sv1.zzzz());
      const sse3f v2(sv2.xxxx(), sv2.yyyy(), sv2.zzzz());
      const sse3f n(sn.xxxx(), sn.yyyy(), sn.zzzz());
      for(uint32 i = 0; i < activeNum; ++i) {
        const uint32 curr = active[i];
        const ssef u = dotZXY(v0, pckt.dir[curr]);
        const ssef v = dotZXY(v1, pckt.dir[curr]);
        const ssef w = dotZXY(v2, pckt.dir[curr]);
        const uint32 us = movemask(u);
        const uint32 vs = movemask(v);
        const uint32 ws = movemask(w);
        const uint32 aperture = (us&vs&ws) | ((us^0xf)&(vs^0xf)&(ws^0xf));
        if (!aperture) continue;
        const ssef n0 = dotZXY(n, pckt.dir[curr]);
        const ssef num = snum.xxxx();
        const ssef t = num / n0;
        const ssei triID(id);
        const sseb inside = unmovemask(aperture);
        const sseb mask = inside & (t<hit.t[curr]) & (t>0.f);
        hit.t[curr]   = select(mask, t, hit.t[curr]);
        hit.u[curr]   = select(mask, u, hit.u[curr]);
        hit.v[curr]   = select(mask, v, hit.v[curr]);
        hit.id0[curr] = select(mask, triID, hit.id0[curr]);
      }
    } else {
      const sse3f a(tri.v[0].x, tri.v[0].y, tri.v[0].z);
      const sse3f b(tri.v[1].x, tri.v[1].y, tri.v[1].z);
      const sse3f c(tri.v[2].x, tri.v[2].y, tri.v[2].z);
      const sse3f n = cross(c - a, b - a);
      const ssei triID(id);

      for(uint32 i = 0; i < activeNum; ++i) {
        const uint32 curr = active[i];
        const sse3f d0 = a - pckt.org[curr];
        const sse3f d1 = b - pckt.org[curr];
        const sse3f d2 = c - pckt.org[curr];
        const sse3f v0 = cross(d1, d2);
        const sse3f v1 = cross(d0, d1);
        const sse3f v2 = cross(d2, d0);
        const ssef num = dot(n, d0);
        const ssef u = dot(v0, pckt.dir[curr]);
        const ssef v = dot(v1, pckt.dir[curr]);
        const ssef w = dot(v2, pckt.dir[curr]);
        const uint32 us = movemask(u);
        const uint32 vs = movemask(v);
        const uint32 ws = movemask(w);
        const uint32 aperture = (us&vs&ws)|((us^0xf)&(vs^0xf)&(ws^0xf));
        if(!aperture) continue;
        const ssef t = num / dot(pckt.dir[curr], n);
        const sseb inside = unmovemask(aperture);
        const sseb mask = inside & (t<hit.t[curr]) & (t>0.f);
        hit.t[curr]   = select(mask, t, hit.t[curr]);
        hit.u[curr]   = select(mask, u, hit.u[curr]);
        hit.v[curr]   = select(mask, v, hit.v[curr]);
        hit.id0[curr] = select(mask, triID, hit.id0[curr]);
      }
    }
  }

  /*! Call stack for packets */
  struct PacketStack
  {
    PacketStack(void) : top(0) {}
    /*! Update top of the stack value */
    INLINE bool pop(void) { return --top >= 0; }
    /*! Push a new element */
    INLINE void push(uint32 nodeID, uint32 first) {
      elem[top].nodeID = nodeID;
      elem[top++].first = first;
    }
    /*! Element of the stack */
    struct Elem {
      uint32 first;
      uint32 nodeID;
    };
    enum { MAX_DEPTH = 128 }; //! Maximum stack depth
    Elem elem[MAX_DEPTH];     //!< All the pushed nodes
    int32 top;                //!< Current size of the stack
  };

  template <typename T>
  void BVH2<T>::traverse(const RayPacket &pckt, PacketHit &hit) const
  {
    PacketStack stack;
    const uint32 s = movemask(pckt.iasign);
    const uint32 signArray[] = { s&1, (s>>1)&1, (s>>2)&1 };
    stack.push(0,0);

  popNode:
    while (LIKELY(stack.pop())) {
      const uint32 nodeID = stack.elem[stack.top].nodeID;
      uint32 firstActive = stack.elem[stack.top].first;
      const BVH2Node * RESTRICT node = &this->node[nodeID];
      while (LIKELY(!node->isLeaf())) {
        if(!AABBIntersect(*node, pckt, hit, firstActive)) goto popNode;
        const uint32 offset = node->getOffset();
        const uint32 first = signArray[node->getAxis()];
        const uint32 second = first ^ 1;
        stack.push(offset + second, firstActive);
        node = &this->node[offset + first];
      }

      // Intersect BVH leaf and its primitives
      LeafIntersect(*this, *node, pckt, firstActive, hit);
    }
  }

  template <typename T>
  void BVH2<T>::occluded(const RayPacket &pckt, PacketOcclusion &o) const {
    NOT_IMPLEMENTED;
  }

  /*! Explicit instantiation for BVH2s of RTTriangle */
  template void BVH2<RTTriangle>::traverse(const RayPacket&, PacketHit&) const;
  template void BVH2<RTTriangle>::occluded(const RayPacket&, PacketOcclusion&) const;

} /* namespace BKY */


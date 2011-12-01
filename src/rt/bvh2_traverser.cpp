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
#include "bvh2_traverser.hpp"
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
  INLINE void PrimIntersect(const T&, uint32, const ssef&, const sse3f&, Hit&);

  INLINE ssef crossZXY(const ssef &a, const ssef &b) {
    return a*b.yzxx() - a.yzxx()*b;
  }
  INLINE ssef dotZXY(const ssef &a, const ssef &b) {
    return a.xxxx()*b.zzzz() + a.yyyy()*b.xxxx() + a.zzzz()*b.yyyy();
  }
  INLINE ssef dotZXY(const sse3f &a, const sse3f &b) {
    return a.x*b.z + a.y*b.x + a.z * b.y;
  }
  INLINE ssef cross(const ssef &a, const ssef &b) {
    return a.yzxx()*b.zxyy() - a.zxyy()*b.yzxx();
  }

  INLINE void transpose4x3(const ssef &r0, const ssef &r1, const ssef &r2, const ssef &r3,
                           ssef& c0,             ssef& c1,       ssef& c2)
  {
    const ssef l02 = unpacklo(r0,r2);
    const ssef h02 = unpackhi(r0,r2);
    const ssef l13 = unpacklo(r1,r3);
    const ssef h13 = unpackhi(r1,r3);
    c0 = unpacklo(l02,l13);
    c1 = unpackhi(l02,l13);
    c2 = unpacklo(h02,h13);
  }

  /*! Node AABB / ray intersection */
  INLINE bool AABBIntersect(const BVH2Node &node, const ssef &org, const ssef &rdir, float t)
  {
    const ssef lower = ssef::load(&node.pmin.x).xyzz();
    const ssef upper = ssef::load(&node.pmax.x).xyzz();
    const ssef l1 = (lower - org) * rdir;
    const ssef l2 = (upper - org) * rdir;
    const ssef near_v = min(l1,l2);
    const ssef far = max(l1,l2);
    const ssef near = reduce_max(near_v);
    const sseb test = (far >= near) & (far >= 0.f) & (near < t);
    return movemask(test) == 0xf;
  }

  /*! Generic ray / leaf intersection */
  template <typename T>
  INLINE void LeafIntersect
    (const BVH2<T> &bvh, const BVH2Node &node, const ssef &org, const sse3f &dir, Hit &hit)
  {
    const uint32 firstPrim = node.getPrimID();
    const uint32 primNum = node.getPrimNum();
    for(uint32 i = 0; i < primNum; ++i) {
      const uint32 primID = bvh.primID[firstPrim + i];
      PrimIntersect(bvh.prim[primID], primID, org, dir, hit);
    }
  }

  /*! Single ray / Pluecker triangle intersection. For single rays, it is
   *  actually bad (see the remark done about Pluecker intersector for
   *  packets). I should clearly replace it by something else
   */
  template <>
  INLINE void PrimIntersect<RTTriangle>
    (const RTTriangle &tri, uint32 id, const ssef &org, const sse3f &dir, Hit &hit)
  {
    const ssef a(&tri.v[0].x);
    const ssef b(&tri.v[1].x);
    const ssef c(&tri.v[2].x);
    const ssef n(&tri.n.x);
    const ssef d0 = a - org;
    const ssef d1 = b - org;
    const ssef d2 = c - org;
    const ssef num = reduce_add(n * d0);
    const ssef v0 = cross(d1, d2);
    const ssef v1 = cross(d0, d1);
    const ssef v2 = cross(d2, d0);
    sse3f v012n;
    transpose4x3(n, v0, v1, v2, v012n.x, v012n.y, v012n.z);
    ssef tuvw = dot(v012n, dir);
    const int m0 = movemask(sseb(tuvw)) & 0xe;
    if ((m0 != 0xe) & (m0 != 0)) return;

    const ssef t = divss(num, tuvw.m128);
    const ssef t0 = subss(ssef(hit.t), t);
    const ssef m = t | t0;
    if ((movemask(m) & 0x1) == 0) {
      ssef::store(t, &hit.t);
      hit.id0 = id;
    }
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
  void BVH2Traverser<T>::traverse(const Ray &ray, Hit &hit) const
  {
    RayStack stack;
    const ssef org = ssef(&ray.org.x).xyzz();
    const ssef rdir = ssef(&ray.rdir.x).xyzz();
    const sse3f dir(ray.dir.x, ray.dir.y, ray.dir.z);
    const uint32 s = movemask(rdir);
    const uint32 signArray[] = { s&1, (s>>1)&1, (s>>2)&1 };
    stack.push(bvh->node);

  popNode:
    // Recurse until we find a leaf
    while (LIKELY(stack.pop())) {
      const BVH2Node * RESTRICT node = stack.elem[stack.top];
      while (AABBIntersect(*node, org, rdir, hit.t)) {
        if (LIKELY(!node->isLeaf())) {
          const uint32 offset = node->getOffset();
          const uint32 first = signArray[node->getAxis()];
          const uint32 second = first ^ 1;
          stack.push(bvh->node + offset + second);
          node = bvh->node + offset + first;
        } else {
          LeafIntersect(*bvh, *node, org, dir, hit);
          goto popNode;
        }
      }
    }
  }

  template <typename T>
  bool BVH2Traverser<T>::occluded(const Ray &ray) const {
    NOT_IMPLEMENTED;
    return false;
  }

  /*! Explicit instantiation for BVH2s of RTTriangle */
  template void BVH2Traverser<RTTriangle>::traverse(const Ray&, Hit&) const;
  template bool BVH2Traverser<RTTriangle>::occluded(const Ray&) const;

  ///////////////////////////////////////////////////////////////////////////
  /// Ray Packet Routines
  ///////////////////////////////////////////////////////////////////////////

  /*! Generic ray / primitive intersection */
  template <typename T>
  INLINE void PrimIntersect
    (const T &, uint32, const RayPacket&, const uint32*, uint32, PacketHit&);

  /*! Kay-Kajiya AABB intersection */
  INLINE void slab
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
  INLINE int slabIA(const ssef &minOrg, const ssef &maxOrg,
                    const sseb &sign,
                    const ssef &rcpMin, const ssef &rcpMax)
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
  INLINE bool AABBIntersect
    (const BVH2Node &node, const RayPacket &pckt, const PacketHit &hit, uint32 &first)
  {
    // Avoid issues with w unused channel
    const ssef lower = ssef::load(&node.pmin.x).xyzz();
    const ssef upper = ssef::load(&node.pmax.x).xyzz();
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
      for (uint32 i = first; i < pckt.chunkNum; ++i) {
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
      for (uint32 i = first; i < pckt.chunkNum; ++i) {
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
  INLINE bool AABBIntersect
    (const BVH2Node &node, const RayPacket &pckt, const PacketHit &hit,
     uint32 first, uint32 *active, uint32 &activeNum)
  {
    // Avoid issues with w unused channel
    const ssef lower = ssef::load(&node.pmin.x).xyzz();
    const ssef upper = ssef::load(&node.pmax.x).xyzz();
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
      for(uint32 i = first; i < pckt.chunkNum; ++i) {
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
      for (uint32 i = first; i < pckt.chunkNum; ++i) {
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
  INLINE void LeafIntersect
    (const BVH2<T> &bvh, const BVH2Node &node, const RayPacket &pckt, uint32 first, PacketHit &hit)
  {
    uint32 active[RayPacket::chunkNum];
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
    (const RTTriangle &tri, uint32 id, const RayPacket &pckt,
     const uint32 *active, uint32 activeNum, PacketHit &hit)
  {
    if (pckt.properties & RAY_PACKET_CO) {
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
#if 0
      const ssef cru = dotZXY(v0, pckt.crdir);
      const ssef crv = dotZXY(v1, pckt.crdir);
      const ssef crw = dotZXY(v2, pckt.crdir);

      if (movemask(cru) == 0) return;
      if (movemask(crv) == 0) return;
      if (movemask(crw) == 0) return;
#endif

      for(uint32 i = 0; i < activeNum; ++i) {
        const uint32 curr = active[i];
        const ssef u = dotZXY(v0, pckt.dir[curr]);
        const ssef v = dotZXY(v1, pckt.dir[curr]);
        const ssef w = dotZXY(v2, pckt.dir[curr]);
        const uint32 us = movemask(u);
        const uint32 vs = movemask(v);
        const uint32 ws = movemask(w);
        const uint32 aperture = (us&vs&ws) | ((us^0xf)&(vs^0xf)&(ws^0xf));
        //const uint32 aperture = (us&vs&ws);
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
  void BVH2Traverser<T>::traverse(const RayPacket &pckt, PacketHit &hit) const
  {
    PacketStack stack;
    const uint32 s = movemask(pckt.iasign);
    const uint32 signArray[] = { s&1, (s>>1)&1, (s>>2)&1 };
    stack.push(0,0);

  popNode:
    while (LIKELY(stack.pop())) {
      const uint32 nodeID = stack.elem[stack.top].nodeID;
      uint32 firstActive = stack.elem[stack.top].first;
      const BVH2Node * RESTRICT node = &bvh->node[nodeID];
      while (LIKELY(!node->isLeaf())) {
        if(!AABBIntersect(*node, pckt, hit, firstActive)) goto popNode;
        const uint32 offset = node->getOffset();
        const uint32 first = signArray[node->getAxis()];
        const uint32 second = first ^ 1;
        stack.push(offset + second, firstActive);
        node = &bvh->node[offset + first];
      }

      // Intersect BVH leaf and its primitives
      LeafIntersect(*bvh, *node, pckt, firstActive, hit);
    }
  }

  /*! Explicit instantiation for BVH2s of RTTriangle */
  template void BVH2Traverser<RTTriangle>::traverse(const RayPacket&, PacketHit&) const;

} /* namespace BKY */


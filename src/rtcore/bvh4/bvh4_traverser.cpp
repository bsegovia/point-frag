// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
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

#include "bvh4_traverser.hpp"
#include "../common/stack_item.hpp"

namespace pf
{
  void BVH4Traverser::intersect(const Ray& ray, Hit& hit) const
  {
    /*! stack state */
    size_t stackPtr = 1;                             //!< current stack pointer
    int32 popCur  = bvh->root;                       //!< pre-popped top node from the stack
    float popDist = neg_inf;                         //!< pre-popped distance of top node from the stack
    StackItem stack[1+3*BVH4<Triangle4>::maxDepth];  //!< stack of nodes that still need to get traversed

    /*! offsets to select the side that becomes the lower or upper bound */
    const size_t nearX = ray.dir.x >= 0 ? 0*sizeof(ssef) : 1*sizeof(ssef);
    const size_t nearY = ray.dir.y >= 0 ? 2*sizeof(ssef) : 3*sizeof(ssef);
    const size_t nearZ = ray.dir.z >= 0 ? 4*sizeof(ssef) : 5*sizeof(ssef);
    const size_t farX  = nearX ^ 16;
    const size_t farY  = nearY ^ 16;
    const size_t farZ  = nearZ ^ 16;

    /*! load the ray into SIMD registers */
    const sse3f norg(-ray.org.x,-ray.org.y,-ray.org.z);
    const sse3f rdir(ray.rdir.x,ray.rdir.y,ray.rdir.z);
    const ssef rayNear(ray.near);
    ssef rayFar(ray.far);
    hit.t = ray.far;
    const BVH4<Triangle4>::Node* nodes = bvh->nodes;

    while (true)
    {
      /*! pop next node */
      if (__builtin_expect(stackPtr == 0, false)) break;
      stackPtr--;
      int32 cur = popCur;

      /*! if popped node is too far, pop next one */
      if (__builtin_expect(popDist > hit.t, false)) {
        popCur  = stack[stackPtr-1].ofs;
        popDist = stack[stackPtr-1].dist;
        continue;
      }

    next:

      /*! we mostly go into the inner node case */
      if (__builtin_expect(cur >= 0, true))
      {
        /*! single ray intersection with 4 boxes */
        const BVH4<Triangle4>::Node& node = bvh->node(nodes,cur);
        const ssef tNearX = (norg.x + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+nearX)) * rdir.x;
        const ssef tNearY = (norg.y + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+nearY)) * rdir.y;
        const ssef tNearZ = (norg.z + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+nearZ)) * rdir.z;
        const ssef tNear = max(tNearX,tNearY,tNearZ,rayNear);
        const ssef tFarX = (norg.x + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+farX)) * rdir.x;
        const ssef tFarY = (norg.y + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+farY)) * rdir.y;
        const ssef tFarZ = (norg.z + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+farZ)) * rdir.z;
        popCur = stack[stackPtr-1].ofs;      //!< pre-pop of topmost stack item
        popDist = stack[stackPtr-1].dist;    //!< pre-pop of distance of topmost stack item
        const ssef tFar = min(tFarX,tFarY,tFarZ,rayFar);
        size_t _hit = movemask(tNear <= tFar);

        /*! if no child is hit, pop next node */
        if (__builtin_expect(_hit == 0, false))
          continue;

        /*! one child is hit, continue with that child */
        size_t r = __bsf(_hit); _hit = __btc(_hit,r);
        if (__builtin_expect(_hit == 0, true)) {
          cur = node.child[r];
          goto next;
        }

        /*! two children are hit, push far child, and continue with closer child */
        const int32 c0 = node.child[r]; const float d0 = tNear[r];
        r = __bsf(_hit); _hit = __btc(_hit,r);
        const int32 c1 = node.child[r]; const float d1 = tNear[r];
        if (__builtin_expect(_hit == 0, true)) {
          if (d0 < d1) { stack[stackPtr].ofs = c1; stack[stackPtr++].dist = d1; cur = c0; goto next; }
          else         { stack[stackPtr].ofs = c0; stack[stackPtr++].dist = d0; cur = c1; goto next; }
        }

        /*! Here starts the slow path for 3 or 4 hit children. We push
         *  all nodes onto the stack to sort them there. */
        stack[stackPtr].ofs = c0; stack[stackPtr++].dist = d0;
        stack[stackPtr].ofs = c1; stack[stackPtr++].dist = d1;

        /*! three children are hit, push all onto stack and sort 3 stack items, continue with closest child */
        r = __bsf(_hit); _hit = __btc(_hit,r);
        int32 c = node.child[r]; float d = tNear[r]; stack[stackPtr].ofs = c; stack[stackPtr++].dist = d;
        if (__builtin_expect(_hit == 0, true)) {
          sort(stack[stackPtr-1],stack[stackPtr-2],stack[stackPtr-3]);
          cur = stack[stackPtr-1].ofs; stackPtr--;
          goto next;
        }

        /*! four children are hit, push all onto stack and sort 4 stack items, continue with closest child */
        r = __bsf(_hit); _hit = __btc(_hit,r);
        c = node.child[r]; d = tNear[r]; stack[stackPtr].ofs = c; stack[stackPtr++].dist = d;
        sort(stack[stackPtr-1],stack[stackPtr-2],stack[stackPtr-3],stack[stackPtr-4]);
        cur = stack[stackPtr-1].ofs; stackPtr--;
        goto next;
      }

      /*! this is a leaf node */
      else {
        cur ^= 0x80000000;
        const size_t ofs = size_t(cur) >> 5;
        const size_t num = size_t(cur) & 0x1F;
        for (size_t i=ofs; i<ofs+num; i++) bvh->triangles[i].intersect(ray,hit);
        popCur = stack[stackPtr-1].ofs;    //!< pre-pop of topmost stack item
        popDist = stack[stackPtr-1].dist;  //!< pre-pop of distance of topmost stack item
        rayFar = hit.t;
      }
    }
  }

  bool BVH4Traverser::occluded(const Ray& ray) const
  {
    /*! stack state */
    size_t stackPtr = 1;                       //!< current stack pointer
    int stack[1+3*BVH4<Triangle4>::maxDepth];  //!< stack of nodes that still need to get traversed
    stack[0] = bvh->root;                      //!< push first node onto stack

    /*! offsets to select the side that becomes the lower or upper bound */
    const size_t nearX = (ray.dir.x >= 0) ? 0*sizeof(ssef) : 1*sizeof(ssef);
    const size_t nearY = (ray.dir.y >= 0) ? 2*sizeof(ssef) : 3*sizeof(ssef);
    const size_t nearZ = (ray.dir.z >= 0) ? 4*sizeof(ssef) : 5*sizeof(ssef);
    const size_t farX  = nearX ^ 16;
    const size_t farY  = nearY ^ 16;
    const size_t farZ  = nearZ ^ 16;

    /*! load the ray into SIMD registers */
    const sse3f norg(-ray.org.x,-ray.org.y,-ray.org.z);
    const sse3f rdir(ray.rdir.x,ray.rdir.y,ray.rdir.z);
    const ssef rayNear(ray.near);
    const ssef rayFar(ray.far);
    const BVH4<Triangle4>::Node* nodes = &bvh->nodes[0];

    /*! pop node from stack */
    while (stackPtr--)
    {
      int32 cur = stack[stackPtr];

      /*! this is an inner node */
      if (__builtin_expect(cur >= 0, true))
      {
        /*! single ray intersection with 4 boxes */
        const BVH4<Triangle4>::Node& node = bvh->node(nodes,cur);
        ssef tNearX = (norg.x + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+nearX)) * rdir.x;
        ssef tNearY = (norg.y + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+nearY)) * rdir.y;
        ssef tNearZ = (norg.z + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+nearZ)) * rdir.z;
        ssef tNear = max(tNearX,tNearY,tNearZ,rayNear);
        ssef tFarX = (norg.x + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+farX)) * rdir.x;
        ssef tFarY = (norg.y + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+farY)) * rdir.y;
        ssef tFarZ = (norg.z + *(ssef*)((const char*)nodes+BVH4<Triangle4>::offsetFactor*size_t(cur)+farZ)) * rdir.z;
        ssef tFar = min(tFarX,tFarY,tFarZ,rayFar);
        size_t _hit = movemask(tNear <= tFar);

        /*! push hit nodes onto stack */
        if (__builtin_expect(_hit == 0, true)) continue;
        size_t r = __bsf(_hit); _hit = __btc(_hit,r);
        stack[stackPtr++] = node.child[r];
        if (__builtin_expect(_hit == 0, true)) continue;
        r = __bsf(_hit); _hit = __btc(_hit,r);
        stack[stackPtr++] = node.child[r];
        if (__builtin_expect(_hit == 0, true)) continue;
        r = __bsf(_hit); _hit = __btc(_hit,r);
        stack[stackPtr++] = node.child[r];
        if (__builtin_expect(_hit == 0, true)) continue;
        r = __bsf(_hit); _hit = __btc(_hit,r);
        stack[stackPtr++] = node.child[r];
      }

      /*! this is a leaf node */
      else {
        cur ^= 0x80000000;
        const size_t ofs = size_t(cur) >> 5;
        const size_t num = size_t(cur) & 0x1F;
        for (size_t i=ofs; i<ofs+num; i++)
          if (bvh->triangles[i].occluded(ray))
            return true;
      }
    }
    return false;
  }
}

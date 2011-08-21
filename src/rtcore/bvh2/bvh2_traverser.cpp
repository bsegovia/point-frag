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

#include "bvh2_traverser.hpp"

namespace pf
{
  void BVH2Traverser::intersect(const Ray& ray, Hit& hit) const
  {
    /*! stack state */
    int stackPtr = 0;                        //!< current stack pointer
    int stack[1+BVH2<Triangle4>::maxDepth];  //!< stack of nodes that still need to get traversed
    float dist[1+BVH2<Triangle4>::maxDepth]; //!< distance of nodes on the stack
    int cur = bvh->root;                     //!< in cur we track the ID of the current node

    /*! precomputed shuffles, to switch lower and upper bounds depending on ray direction */
    const ssei identity = _mm_set_epi8(15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1, 0);
    const ssei swap     = _mm_set_epi8( 7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9, 8);
    const ssei shuffleX = ray.dir.x >= 0 ? identity : swap;
    const ssei shuffleY = ray.dir.y >= 0 ? identity : swap;
    const ssei shuffleZ = ray.dir.z >= 0 ? identity : swap;

    /*! load the ray into SIMD registers */
    const ssei pn = ssei(0x00000000,0x00000000,0x80000000,0x80000000);
    const sse3f norg(-ray.org.x,-ray.org.y,-ray.org.z);
    const sse3f rdir = sse3f(ssef(ray.rdir.x) ^ pn, ssef(ray.rdir.y) ^ pn, ssef(ray.rdir.z) ^ pn);
    ssef nearFar(ray.near, ray.near, -ray.far, -ray.far);
    hit.t = ray.far;
    BVH2<Triangle4>::Node* nodes = bvh->nodes;

    while (true)
    {
      /*! downtraversal loop */
      while (__builtin_expect(cur >= 0, true))
      {
        /*! single ray intersection with box of both children. */
        const BVH2<Triangle4>::Node& node = bvh->node(nodes,cur);
        const ssef tNearFarX = (shuffle8(node.lower_upper_x,shuffleX) + norg.x) * rdir.x;
        const ssef tNearFarY = (shuffle8(node.lower_upper_y,shuffleY) + norg.y) * rdir.y;
        const ssef tNearFarZ = (shuffle8(node.lower_upper_z,shuffleZ) + norg.z) * rdir.z;
        const ssef tNearFar = max(tNearFarX,tNearFarY,tNearFarZ,nearFar) ^ pn;
        const sseb lrhit = tNearFar <= shuffle8(tNearFar,swap);

        /*! if two children hit, push far node onto stack and continue with closer node */
        if (__builtin_expect(lrhit[0] != 0 && lrhit[1] != 0, true)) {
          if (tNearFar[0] < tNearFar[1]) { stack[stackPtr] = node.child[1]; dist[stackPtr++] = tNearFar[1]; cur = node.child[0]; }
          else                           { stack[stackPtr] = node.child[0]; dist[stackPtr++] = tNearFar[0]; cur = node.child[1]; }
        }

        /*! if one child hit, continue with that child */
        else {
          if      (__builtin_expect(lrhit[0] != 0, true)) cur = node.child[0];
          else if (__builtin_expect(lrhit[1] != 0, true)) cur = node.child[1];
          else goto pop_node;
        }
      }

      /*! leaf node, intersect all triangles */
      {
        cur ^= 0x80000000;
        const size_t ofs = size_t(cur) >> 5;
        const size_t num = size_t(cur) & 0x1F;
        for (size_t i=ofs; i<ofs+num; i++) bvh->triangles[i].intersect(ray,hit);
        nearFar = shuffle<0,1,2,3>(nearFar,-hit.t);
      }

      /*! pop next node from stack */
pop_node:
      if (__builtin_expect(stackPtr == 0, false)) break;
      cur = stack[--stackPtr];
      if (__builtin_expect(dist[stackPtr] > ray.far, false)) goto pop_node;
    }
  }

  bool BVH2Traverser::occluded(const Ray& ray) const
  {
    /*! stack state */
    int stackPtr = 0;                         //!< current stack pointer
    int stack[1+BVH2<Triangle4>::maxDepth];   //!< stack of nodes that still need to get traversed
    int cur = bvh->root;                      //!< in cur we track the ID of the current node

    /*! precomputed shuffles, to switch lower and upper bounds depending on ray direction */
    const ssei identity = _mm_set_epi8(15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1, 0);
    const ssei swap     = _mm_set_epi8( 7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9, 8);
    const ssei shuffleX = ray.dir.x >= 0 ? identity : swap;
    const ssei shuffleY = ray.dir.y >= 0 ? identity : swap;
    const ssei shuffleZ = ray.dir.z >= 0 ? identity : swap;

    /*! load the ray into SIMD registers */
    const ssei pn = ssei(0x00000000,0x00000000,0x80000000,0x80000000);
    const sse3f norg(-ray.org.x,-ray.org.y,-ray.org.z);
    const sse3f rdir = sse3f(ssef(ray.rdir.x) ^ pn, ssef(ray.rdir.y) ^ pn, ssef(ray.rdir.z) ^ pn);
    ssef nearFar(ray.near, ray.near, -ray.far, -ray.far);
    BVH2<Triangle4>::Node* nodes = bvh->nodes;

    while (true)
    {
      /*! this is an inner node */
      while (__builtin_expect(cur >= 0, true))
      {
        /*! Single ray intersection with box of both children. See bvh2.h for node layout. */
        const BVH2<Triangle4>::Node& node = bvh->node(nodes,cur);
        const ssef tNearFarX = (shuffle8(node.lower_upper_x,shuffleX) + norg.x) * rdir.x;
        const ssef tNearFarY = (shuffle8(node.lower_upper_y,shuffleY) + norg.y) * rdir.y;
        const ssef tNearFarZ = (shuffle8(node.lower_upper_z,shuffleZ) + norg.z) * rdir.z;
        const ssef tNearFar = max(tNearFarX,tNearFarY,tNearFarZ,nearFar) ^ pn;
        const sseb lrhit = tNearFar <= shuffle8(tNearFar,swap);

        /*! if two children hit, push far node onto stack and continue with closer node */
        if (__builtin_expect(lrhit[0] != 0 && lrhit[1] != 0, true)) {
          if (tNearFar[0] < tNearFar[1]) { stack[stackPtr++] = node.child[1]; cur = node.child[0]; }
          else                           { stack[stackPtr++] = node.child[0]; cur = node.child[1]; }
        }

        /*! if one child hit, continue with that child */
        else {
          if      (lrhit[0] != 0) cur = node.child[0];
          else if (lrhit[1] != 0) cur = node.child[1];
          else goto pop_node;
        }
      }

      /*! leaf node, intersect all triangles */
      {
        cur ^= 0x80000000;
        const size_t ofs = size_t(cur) >> 5;
        const size_t num = size_t(cur) & 0x1F;
        for (size_t i=ofs; i<ofs+num; i++)
          if (bvh->triangles[i].occluded(ray))
            return true;
      }

      /*! pop next node from stack */
pop_node:
      if (__builtin_expect(stackPtr == 0, false)) break;
      cur = stack[--stackPtr];
    }
    return false;
  }
}

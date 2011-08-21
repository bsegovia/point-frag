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

#ifndef __PF_ACCEL_TRIANGLE4_H__
#define __PF_ACCEL_TRIANGLE4_H__

#include "../ray.hpp"
#include "../hit.hpp"

namespace pf
{
  /*! Intersector for 4 triangles. Implements a modified version of
   *  the Moeller Trumbore intersector in order to take advantage of
   *  a precomputed geometry normal. */
  struct Triangle4
  {
    /*! Default constructor. */
    INLINE Triangle4 () {}

    /*! Construction from vertices and IDs. */
    INLINE Triangle4 (const sse3f& v0, const sse3f& v1, const sse3f& v2, const ssei& id0, const ssei& id1)
      : v0(v0), e1(v0-v1), e2(v2-v0), Ng(cross(e1,e2)), id0(id0), id1(id1) {}

    /*! Returns a mask that tells which triangles are valid. */
    INLINE sseb valid() const { return id0 != ssei(-1); }

    /*! Returns the number of stored triangles. */
    INLINE size_t size() const {
      size_t r = 0; for (size_t i=0; i<4; i++) if (valid()[i]) r++; return r;
    }

    /*! Intersect a ray with the 4 triangles and updates the hit. */
    INLINE void intersect(const Ray& ray, Hit& hit) const
    {
      sse3f O = sse3f(ray.org);
      sse3f D = sse3f(ray.dir);
      sse3f C = v0 - O;
      sse3f R = cross(D,C);
      ssef det = dot(Ng,D);
      ssef T = dot(Ng,C);
      ssef U = dot(R,e2);
      ssef V = dot(R,e1);
      ssef absDet = abs(det);
      ssei signDet = _mm_castps_si128(det) & ssei(0x80000000);
      ssef _t = _mm_castsi128_ps(ssei(_mm_castps_si128(T)) ^ signDet);
      ssef _u = _mm_castsi128_ps(ssei(_mm_castps_si128(U)) ^ signDet);
      ssef _v = _mm_castsi128_ps(ssei(_mm_castps_si128(V)) ^ signDet);
      ssef _w = absDet-_u-_v;
      sseb mask = valid() & (det != ssef(zero)) & (_t >= absDet*ssef(ray.near)) & (absDet*ssef(hit.t) >= _t) & (min(_u,_v,_w) >= ssef(zero));
      if (none(mask)) return;
      ssef rcpAbsDet = rcp(absDet);
      ssef t = _t * rcpAbsDet;
      ssef u = _u * rcpAbsDet;
      ssef v = _v * rcpAbsDet;
      ssef __t = select(mask,t,ssef(inf));
      size_t tri = __bsf(movemask(__t == reduce_min(__t)));
      hit.t = t[tri];
      hit.u = u[tri];
      hit.v = v[tri];
      hit.id0 = id0[tri];
      hit.id1 = id1[tri];
    }

    /*! Test if the ray is occluded by one of the triangles. */
    INLINE bool occluded(const Ray& ray) const
    {
      sse3f O = sse3f(ray.org);
      sse3f D = sse3f(ray.dir);
      sse3f C = v0 - O;
      sse3f R = cross(D,C);
      ssef det = dot(Ng,D);
      ssef T = dot(Ng,C);
      ssef U = dot(R,e2);
      ssef V = dot(R,e1);
      ssef absDet = abs(det);
      ssei signDet = _mm_castps_si128(det) & ssei(0x80000000);
      ssef _t = _mm_castsi128_ps(ssei(_mm_castps_si128(T)) ^ signDet);
      ssef _u = _mm_castsi128_ps(ssei(_mm_castps_si128(U)) ^ signDet);
      ssef _v = _mm_castsi128_ps(ssei(_mm_castps_si128(V)) ^ signDet);
      ssef _w = absDet-_u-_v;
      sseb hit = valid() & (det != ssef(zero)) & (_t >= absDet*ssef(ray.near)) & (absDet*ssef(ray.far) >= _t) & (min(_u,_v,_w) >= ssef(zero));
      return any(hit);
    }

  public:
    sse3f v0;      //!< Base vertex of the triangles.
    sse3f e1;      //!< 1st edge of the triangles (v0-v1).
    sse3f e2;      //!< 2nd edge of the triangles (v2-v0).
    sse3f Ng;      //!< Geometry normal of the triangles.
    ssei id0;      //!< 1st user ID.
    ssei id1;      //!< 2nd user ID.
  };
}

#endif



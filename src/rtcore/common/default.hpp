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

#ifndef __PF_RTCORE_DEFAULT_H__
#define __PF_RTCORE_DEFAULT_H__

#include <cstring>
#include <iostream>
#include <vector>
#include <map>

#include "sys/platform.hpp"
#include "sys/tasking.hpp"
#include "sys/atomic.hpp"
#include "sys/intrinsics.hpp"
#include "sys/vector.hpp"

#include "math/math.hpp"
#include "math/vec.hpp"
#include "math/bbox.hpp"

#include "simd/sse.hpp"

namespace pf
{
  /*! Box to use in the builders. */
  typedef BBox<ssef> Box;

  /*! Computes half surface area of box. */
  INLINE float halfArea(const Box& box) {
    ssef d = size(box);
    ssef a = d*shuffle<1,2,0,3>(d);
    return extract<0>(reduce_add(a));
  }

  typedef vec2<sseb> sse2b;
  typedef vec3<sseb> sse3b;
  typedef vec2<ssei> sse2i;
  typedef vec3<ssei> sse3i;
  typedef vec2<ssef> sse2f;
  typedef vec3<ssef> sse3f;
}
#if 0
#if !defined(__NO_AVX__)

#include "simd/avx.hpp"

namespace pf
{
  typedef vec2<avxb> avx2b;
  typedef vec3<avxb> avx3b;
  typedef vec2<avxi> avx2i;
  typedef vec3<avxi> avx3i;
  typedef vec2<avxf> avx2f;
  typedef vec3<avxf> avx3f;
}
#endif
#endif

#endif

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

#ifndef __PF_SSE_VEC_HPP__
#define __PF_SSE_VEC_HPP__

#include "ssef.hpp"
#include "ssei.hpp"
#include "sseb.hpp"
#include "math/vec.hpp"
namespace pf
{
  typedef vec2<ssef> sse2f;
  typedef vec3<ssef> sse3f;
  typedef vec4<ssef> sse4f;
  typedef vec2<ssei> sse2i;
  typedef vec3<ssei> sse3i;
  typedef vec4<ssei> sse4i;
  typedef vec2<sseb> sse2b;
  typedef vec3<sseb> sse3b;
  typedef vec4<sseb> sse4b;
} /* namespace pf */

#endif /* __PF_SSE_VEC_HPP__ */


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

#ifndef __PF_SSE_SWIZZLE_HPP__
#define __PF_SSE_SWIZZLE_HPP__

#define SWZ_VEC4_0(RET_TYPE, FUNC, PREFIX, OP0, OP1, OP2, OP3)      \
  INLINE RET_TYPE FUNC##PREFIX(void) const {                        \
    return this->swizzle<OP0, OP1, OP2, OP3>();                     \
  }

#define SWZ_VEC4_1(RET_TYPE, FUNC, PREFIX, OP0, OP1, OP2)           \
  SWZ_VEC4_0(RET_TYPE, FUNC##PREFIX, x, OP0, OP1, OP2, 0)           \
  SWZ_VEC4_0(RET_TYPE, FUNC##PREFIX, y, OP0, OP1, OP2, 1)           \
  SWZ_VEC4_0(RET_TYPE, FUNC##PREFIX, z, OP0, OP1, OP2, 2)           \
  SWZ_VEC4_0(RET_TYPE, FUNC##PREFIX, w, OP0, OP1, OP2, 3)

#define SWZ_VEC4_2(RET_TYPE, FUNC, PREFIX, OP0, OP1)                \
  SWZ_VEC4_1(RET_TYPE, FUNC##PREFIX, x, OP0, OP1, 0)                \
  SWZ_VEC4_1(RET_TYPE, FUNC##PREFIX, y, OP0, OP1, 1)                \
  SWZ_VEC4_1(RET_TYPE, FUNC##PREFIX, z, OP0, OP1, 2)                \
  SWZ_VEC4_1(RET_TYPE, FUNC##PREFIX, w, OP0, OP1, 3)

/*! Generate the swizzle functions xxxx, yyyy, zzzz and so on */
#define GEN_VEC4_SWZ_FUNCS(RET_TYPE)                                \
  SWZ_VEC4_2(RET_TYPE, x, x, 0, 0)                                  \
  SWZ_VEC4_2(RET_TYPE, x, y, 0, 1)                                  \
  SWZ_VEC4_2(RET_TYPE, x, z, 0, 2)                                  \
  SWZ_VEC4_2(RET_TYPE, x, w, 0, 3)                                  \
  SWZ_VEC4_2(RET_TYPE, y, x, 1, 0)                                  \
  SWZ_VEC4_2(RET_TYPE, y, y, 1, 1)                                  \
  SWZ_VEC4_2(RET_TYPE, y, z, 1, 2)                                  \
  SWZ_VEC4_2(RET_TYPE, y, w, 1, 3)                                  \
  SWZ_VEC4_2(RET_TYPE, z, x, 2, 0)                                  \
  SWZ_VEC4_2(RET_TYPE, z, y, 2, 1)                                  \
  SWZ_VEC4_2(RET_TYPE, z, z, 2, 2)                                  \
  SWZ_VEC4_2(RET_TYPE, z, w, 2, 3)                                  \
  SWZ_VEC4_2(RET_TYPE, w, x, 3, 0)                                  \
  SWZ_VEC4_2(RET_TYPE, w, y, 3, 1)                                  \
  SWZ_VEC4_2(RET_TYPE, w, z, 3, 2)                                  \
  SWZ_VEC4_2(RET_TYPE, w, w, 3, 3)

#endif /* __PF_SSE_SWIZZLE_HPP__ */


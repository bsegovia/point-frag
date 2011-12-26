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

#ifndef __PF_SSEB_H__
#define __PF_SSEB_H__

#include "sse.hpp"

namespace pf
{
  /*! 4-wide SSE bool type. */
  struct sseb
  {
    union { __m128 m128; int32 v[4]; };

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////

    typedef sseb Mask;

    INLINE sseb(void) {}
    INLINE sseb(const sseb& other) { m128 = other.m128; }
    INLINE sseb& operator= (const sseb& other) { m128 = other.m128; return *this; }

    INLINE sseb(__m128  x) : m128(x) {}
    INLINE sseb(__m128i x) : m128(_mm_castsi128_ps(x)) {}
    INLINE sseb(__m128d x) : m128(_mm_castpd_ps(x)) {}

    INLINE sseb(bool x)
      : m128(x ? _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128())) : _mm_setzero_ps()) {}
    INLINE sseb(bool x0, bool x1, bool x2, bool x3)
      : m128(_mm_lookupmask_ps[(size_t(x3) << 3) | (size_t(x2) << 2) | (size_t(x1) << 1) | size_t(x0)]) {}

    INLINE operator const __m128&(void) const { return m128; }
    INLINE operator const __m128i(void) const { return _mm_castps_si128(m128); }
    INLINE operator const __m128d(void) const { return _mm_castps_pd(m128); }

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    INLINE sseb(FalseTy) : m128(_mm_setzero_ps()) {}
    INLINE sseb(TrueTy)  : m128(_mm_castsi128_ps(_mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()))) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// Properties
    ////////////////////////////////////////////////////////////////////////////////

    INLINE bool   operator [](const size_t index) const { PF_ASSERT(index < 4); return (_mm_movemask_ps(m128) >> index) & 1; }
    INLINE int32& operator [](const size_t index) { PF_ASSERT(index < 4); return this->v[index]; }
    PF_STRUCT(sseb);
  };

  INLINE const sseb operator !(const sseb& a) { return _mm_xor_ps(a, sseb(True)); }
  INLINE const sseb operator &(const sseb& a, const sseb& b) { return _mm_and_ps(a, b); }
  INLINE const sseb operator |(const sseb& a, const sseb& b) { return _mm_or_ps (a, b); }
  INLINE const sseb operator ^(const sseb& a, const sseb& b) { return _mm_xor_ps(a, b); }

  INLINE sseb operator &=(sseb& a, const sseb& b) { return a = a & b; }
  INLINE sseb operator |=(sseb& a, const sseb& b) { return a = a | b; }
  INLINE sseb operator ^=(sseb& a, const sseb& b) { return a = a ^ b; }

  INLINE const sseb operator !=(const sseb& a, const sseb& b) { return _mm_xor_ps(a, b); }
  INLINE const sseb operator ==(const sseb& a, const sseb& b) { return _mm_cmpeq_epi32(a, b); }

  INLINE bool reduce_and(const sseb& a) { return _mm_movemask_ps(a) == 0xf; }
  INLINE bool reduce_or (const sseb& a) { return _mm_movemask_ps(a) != 0x0; }
  INLINE bool all       (const sseb& b) { return _mm_movemask_ps(b) == 0xf; }
  INLINE bool any       (const sseb& b) { return _mm_movemask_ps(b) != 0x0; }
  INLINE bool none      (const sseb& b) { return _mm_movemask_ps(b) == 0x0; }

  INLINE size_t movemask(const sseb& a) { return _mm_movemask_ps(a); }
  INLINE sseb unmovemask(size_t m)      { return _mm_lookupmask_ps[m]; }

  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3>
  INLINE sseb shuffle(const sseb& a) {
    return sseb(_mm_shuffle_epi32(_mm_castps_si128(a.m128), _MM_SHUFFLE(index_3, index_2, index_1, index_0)));
  }

  //template<> INLINE const sseb shuffle<0, 0, 2, 2>(const sseb& a) { return _mm_moveldup_ps(a); }
  //template<> INLINE const sseb shuffle<1, 1, 3, 3>(const sseb& a) { return _mm_movehdup_ps(a); }
  //template<> INLINE const sseb shuffle<0, 1, 0, 1>(const sseb& a) { return _mm_movedup_pd(a); }
  template<size_t index> INLINE const sseb expand(const sseb& a) { return shuffle<index, index, index, index>(a); }

  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3>
    INLINE const sseb shuffle(const sseb& a, const sseb& b)
  {
    return _mm_shuffle_ps(a, b, _MM_SHUFFLE(index_3, index_2, index_1, index_0));
  }

  INLINE const sseb unpacklo(const sseb& a, const sseb& b) { return _mm_unpacklo_ps(a, b); }
  INLINE const sseb unpackhi(const sseb& a, const sseb& b) { return _mm_unpackhi_ps(a, b); }

  inline std::ostream& operator<<(std::ostream& cout, const sseb& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ">";
  }
}

#endif

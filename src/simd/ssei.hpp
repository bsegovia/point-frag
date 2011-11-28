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

#ifndef __PF_SSEI_H__
#define __PF_SSEI_H__

#include "sse.hpp"

namespace pf
{
  /*! 4-wide SSE integer type. */
  struct ssei
  {
    union { __m128i m128; int32 v[4]; };

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////

    typedef sseb Mask;

    INLINE ssei           () {}
    INLINE ssei           (const ssei& other) { m128 = other.m128; }
    INLINE ssei& operator=(const ssei& other) { m128 = other.m128; return *this; }

    INLINE ssei(const __m128i a) : m128(a) {}
    INLINE explicit ssei(const __m128 a) : m128(_mm_cvtps_epi32(a)) {}

    INLINE explicit ssei(const int32* const a) : m128(_mm_loadu_si128((__m128i*)a)) {}
    INLINE ssei(const int32 a) : m128(_mm_set1_epi32(a)) {}
    INLINE ssei(const int32 a, const int32 b, const int32 c, const int32 d) : m128(_mm_set_epi32(d, c, b, a)) {}

    INLINE operator const __m128i&(void) const { return m128; }
    INLINE operator       __m128i&(void)       { return m128; }

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    INLINE ssei(ZeroTy  ) : m128(_mm_setzero_si128()) {}
    INLINE ssei(OneTy   ) : m128(_mm_set_epi32(1, 1, 1, 1)) {}
    INLINE ssei(PosInfTy) : m128(_mm_set_epi32(pos_inf, pos_inf, pos_inf, pos_inf)) {}
    INLINE ssei(NegInfTy) : m128(_mm_set_epi32(neg_inf, neg_inf, neg_inf, neg_inf)) {}
    INLINE ssei(StepTy)   : m128(_mm_set_epi32(3, 2, 1, 0)) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// Properties
    ////////////////////////////////////////////////////////////////////////////////

    INLINE const int32& operator [](const size_t index) const { assert(index < 4); return v[index]; }
    INLINE       int32& operator [](const size_t index)       { assert(index < 4); return v[index]; }
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const ssei operator +(const ssei& a) { return a; }
  INLINE const ssei operator -(const ssei& a) { return _mm_sub_epi32(_mm_setzero_si128(), a.m128); }
  //INLINE const ssei abs(const ssei& a) { return _mm_abs_epi32(a.m128); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const ssei operator  +(const ssei& a, const ssei& b) { return _mm_add_epi32(a.m128, b.m128); }
  INLINE const ssei operator  -(const ssei& a, const ssei& b) { return _mm_sub_epi32(a.m128, b.m128); }
  INLINE const ssei operator  *(const ssei& a, const ssei& b) { return _mm_mullo_epi32(a.m128, b.m128); }
  INLINE const ssei operator  &(const ssei& a, const ssei& b) { return _mm_and_si128(a.m128, b.m128); }
  INLINE const ssei operator  |(const ssei& a, const ssei& b) { return _mm_or_si128(a.m128, b.m128); }
  INLINE const ssei operator  ^(const ssei& a, const ssei& b) { return _mm_xor_si128(a.m128, b.m128); }
  INLINE const ssei operator <<(const ssei& a, const int32 bits) { return _mm_slli_epi32(a.m128, bits); }
  INLINE const ssei operator >>(const ssei& a, const int32 bits) { return _mm_srai_epi32(a.m128, bits); }

  INLINE const ssei operator  *(const ssei& a, const int32 b) { return a * ssei(b); }
  INLINE const ssei operator  *(const int32 a, const ssei& b) { return ssei(a) * b; }
  INLINE const ssei operator  &(const ssei& a, const int32 b) { return a & ssei(b); }
  INLINE const ssei operator  |(const ssei& a, const int32 b) { return a | ssei(b); }
  INLINE const ssei operator  ^(const ssei& a, const int32 b) { return a ^ ssei(b); }

  INLINE ssei& operator  +=(ssei& a, const ssei& b) { return a = a  + b; }
  INLINE ssei& operator  -=(ssei& a, const ssei& b) { return a = a  - b; }
  INLINE ssei& operator  *=(ssei& a, const ssei& b) { return a = a  * b; }
  INLINE ssei& operator  &=(ssei& a, const ssei& b) { return a = a  & b; }
  INLINE ssei& operator  *=(ssei& a, const int32 b) { return a = a  * b; }
  INLINE ssei& operator  &=(ssei& a, const int32 b) { return a = a  & b; }
  INLINE ssei& operator <<=(ssei& a, const int32 b) { return a = a << b; }
  INLINE ssei& operator >>=(ssei& a, const int32 b) { return a = a >> b; }

  INLINE const ssei min(const ssei& a, const ssei& b) { return _mm_min_epi32(a.m128, b.m128); }
  INLINE const ssei max(const ssei& a, const ssei& b) { return _mm_max_epi32(a.m128, b.m128); }

  INLINE const ssei sra (const ssei& a, const int32 b) { return _mm_srai_epi32(a.m128, b); }
  INLINE const ssei srl (const ssei& a, const int32 b) { return _mm_srli_epi32(a.m128, b); }
  INLINE const ssei rotl(const ssei& a, const int32 b) { return _mm_or_si128(_mm_srli_epi32(a.m128, 32 - b), _mm_slli_epi32(a.m128, b)); }
  INLINE const ssei rotr(const ssei& a, const int32 b) { return _mm_or_si128(_mm_slli_epi32(a.m128, 32 - b), _mm_srli_epi32(a.m128, b)); }


  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const sseb operator ==(const ssei& a, const ssei& b) { return _mm_cmpeq_epi32 (a.m128, b.m128); }
  INLINE const sseb operator < (const ssei& a, const ssei& b) { return _mm_cmplt_epi32 (a.m128, b.m128); }
  INLINE const sseb operator > (const ssei& a, const ssei& b) { return _mm_cmpgt_epi32 (a.m128, b.m128); }
  INLINE const sseb operator !=(const ssei& a, const ssei& b) { return !(a == b); }
  INLINE const sseb operator >=(const ssei& a, const ssei& b) { return !(a <  b); }
  INLINE const sseb operator <=(const ssei& a, const ssei& b) { return !(a >  b); }
  INLINE const sseb cmpa(const ssei& a, const ssei& b) { return _mm_cmpgt_epi32(_mm_xor_si128(a.m128, _mm_set1_epi32(0x80000000)), _mm_xor_si128(b.m128, _mm_set1_epi32(0x80000000))); }

  /*! workaround for compiler bug in VS2008 */
#if defined(_MSC_VER)
  INLINE const ssei select(const sseb& mask, const ssei& a, const ssei& b) { 
    return _mm_castps_si128(_mm_or_ps(_mm_and_ps(mask, _mm_castsi128_ps(a)), _mm_andnot_ps(mask, _mm_castsi128_ps(b)))); 
  }
#else
  INLINE const ssei select(const sseb& mask, const ssei& a, const ssei& b) { 
    return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(b), _mm_castsi128_ps(a), mask)); 
  }
#endif

  ////////////////////////////////////////////////////////////////////////////////
  // Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////

  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3>
  INLINE const ssei shuffle(const ssei& a)
  {
    return _mm_shuffle_epi32(a, _MM_SHUFFLE(index_3, index_2, index_1, index_0));
  }

  //template<> INLINE const ssei shuffle<0, 0, 2, 2>(const ssei& a) { return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(a))); }
  //template<> INLINE const ssei shuffle<1, 1, 3, 3>(const ssei& a) { return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(a))); }
  //template<> INLINE const ssei shuffle<0, 1, 0, 1>(const ssei& a) { return _mm_castpd_si128(_mm_movedup_pd (_mm_castsi128_pd(a))); }

  template<size_t index> INLINE const ssei expand(const ssei& b) { return shuffle<index, index, index, index>(b); }
  template<size_t dst> INLINE const ssei insert(const ssei& a, const int32 b) { return _mm_insert_epi32(a, b, dst); }

  /*! workaround for compiler bug in VS2008 */
#if defined(_MSC_VER)
  template<size_t src> INLINE int extract(const ssei& b) { return b[src]; }
#else
  template<size_t src> INLINE int extract(const ssei& b) { return _mm_extract_epi32(b, src); }
#endif

  INLINE ssei unpacklo(const ssei& a, const ssei& b) { return _mm_castps_si128(_mm_unpacklo_ps(_mm_castsi128_ps(a.m128), _mm_castsi128_ps(b.m128))); }
  INLINE ssei unpackhi(const ssei& a, const ssei& b) { return _mm_castps_si128(_mm_unpackhi_ps(_mm_castsi128_ps(a.m128), _mm_castsi128_ps(b.m128))); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  inline std::ostream& operator<<(std::ostream& cout, const ssei& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ">";
  }
}

#endif

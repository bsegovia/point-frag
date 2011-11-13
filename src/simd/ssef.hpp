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

#ifndef __PF_SSEF_H__
#define __PF_SSEF_H__

#include "sse.hpp"

namespace pf
{
  /*! 4-wide SSE float type. */
  struct ssef
  {
    union { __m128 m128; float v[4]; int i[4]; };

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////

    typedef sseb Mask;

    INLINE ssef           () {}
    INLINE ssef           (const ssef& other) { m128 = other.m128; }
    INLINE ssef& operator=(const ssef& other) { m128 = other.m128; return *this; }

    INLINE ssef(const __m128 a) : m128(a) {}
    INLINE explicit ssef(const __m128i a) : m128(_mm_cvtepi32_ps(a)) {}

    INLINE explicit ssef(const float* const a) : m128(_mm_loadu_ps(a)) {}
    INLINE ssef(const float a) : m128(_mm_set1_ps(a)) {}
    INLINE ssef(const float a, const float b, const float c, const float d) : m128(_mm_set_ps(d, c, b, a)) {}

    INLINE operator const __m128&(void) const { return m128; }
    INLINE operator       __m128&(void)       { return m128; }

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    INLINE ssef(ZeroTy  ) : m128(_mm_set1_ps(0.0f)) {}
    INLINE ssef(OneTy   ) : m128(_mm_set1_ps(1.0f)) {}
    INLINE ssef(PosInfTy) : m128(_mm_set1_ps(pos_inf)) {}
    INLINE ssef(NegInfTy) : m128(_mm_set1_ps(neg_inf)) {}
    INLINE ssef(StepTy  ) : m128(_mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f)) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// Properties
    ////////////////////////////////////////////////////////////////////////////////

    INLINE const float& operator [](const size_t index) const { assert(index < 4); return v[index]; }
    INLINE       float& operator [](const size_t index)       { assert(index < 4); return v[index]; }
  };


  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const ssef operator +(const ssef& a) { return a; }
  INLINE const ssef operator -(const ssef& a) {
    const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
    return _mm_xor_ps(a.m128, mask);
  }

  INLINE const ssef abs  (const ssef& a) {
    const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    return _mm_and_ps(a.m128, mask);
  }

  INLINE const ssef sign (const ssef& a) { return _mm_blendv_ps(ssef(one), -ssef(one), _mm_cmplt_ps (a,ssef(zero))); }
  INLINE const ssef rcp  (const ssef& a) {
    const ssef r = _mm_rcp_ps(a.m128);
    return _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(r, r), a));
  }

  INLINE const ssef sqrt (const ssef& a) { return _mm_sqrt_ps(a.m128); }
  INLINE const ssef sqr  (const ssef& a) { return _mm_mul_ps(a,a); }
  INLINE const ssef rsqrt(const ssef& a) {
    const ssef r = _mm_rsqrt_ps(a.m128);
    return _mm_add_ps(_mm_mul_ps(_mm_set_ps(1.5f, 1.5f, 1.5f, 1.5f), r),
                      _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set_ps(-0.5f, -0.5f, -0.5f, -0.5f)), r), _mm_mul_ps(r, r)));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const ssef operator +(const ssef& a, const ssef& b) { return _mm_add_ps(a.m128, b.m128); }
  INLINE const ssef operator +(const ssef& a, const float b) { return a + ssef(b); }
  INLINE const ssef operator +(const float a, const ssef& b) { return ssef(a) + b; }

  INLINE const ssef operator -(const ssef& a, const ssef& b) { return _mm_sub_ps(a.m128, b.m128); }
  INLINE const ssef operator -(const ssef& a, const float b) { return a - ssef(b); }
  INLINE const ssef operator -(const float a, const ssef& b) { return ssef(a) - b; }

  INLINE const ssef operator *(const ssef& a, const ssef& b) { return _mm_mul_ps(a.m128, b.m128); }
  INLINE const ssef operator *(const ssef& a, const float b) { return a * ssef(b); }
  INLINE const ssef operator *(const float a, const ssef& b) { return ssef(a) * b; }

  INLINE const ssef operator /(const ssef& a, const ssef& b) { return a * rcp(b); }
  INLINE const ssef operator /(const ssef& a, const float b) { return a * rcp(b); }
  INLINE const ssef operator /(const float a, const ssef& b) { return a * rcp(b); }

  INLINE const ssef min(const ssef& a, const ssef& b) { return _mm_min_ps(a.m128,b.m128); }
  INLINE const ssef min(const ssef& a, const float b) { return _mm_min_ps(a.m128,ssef(b)); }
  INLINE const ssef min(const float a, const ssef& b) { return _mm_min_ps(ssef(a),b.m128); }

  INLINE const ssef max(const ssef& a, const ssef& b) { return _mm_max_ps(a.m128,b.m128); }
  INLINE const ssef max(const ssef& a, const float b) { return _mm_max_ps(a.m128,ssef(b)); }
  INLINE const ssef max(const float a, const ssef& b) { return _mm_max_ps(ssef(a),b.m128); }

  INLINE const ssef operator^(const ssef& a, const ssei& b) { return _mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(a.m128),b.m128)); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Assignment Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE ssef& operator +=(ssef& a, const ssef& b) { return a = a + b; }
  INLINE ssef& operator +=(ssef& a, const float b) { return a = a + b; }

  INLINE ssef& operator -=(ssef& a, const ssef& b) { return a = a - b; }
  INLINE ssef& operator -=(ssef& a, const float b) { return a = a - b; }

  INLINE ssef& operator *=(ssef& a, const ssef& b) { return a = a * b; }
  INLINE ssef& operator *=(ssef& a, const float b) { return a = a * b; }

  INLINE ssef& operator /=(ssef& a, const ssef& b) { return a = a / b; }
  INLINE ssef& operator /=(ssef& a, const float b) { return a = a / b; }


  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const sseb operator ==(const ssef& a, const ssef& b) { return _mm_cmpeq_ps (a.m128, b.m128); }
  INLINE const sseb operator !=(const ssef& a, const ssef& b) { return _mm_cmpneq_ps(a.m128, b.m128); }
  INLINE const sseb operator < (const ssef& a, const ssef& b) { return _mm_cmplt_ps (a.m128, b.m128); }
  INLINE const sseb operator <=(const ssef& a, const ssef& b) { return _mm_cmple_ps (a.m128, b.m128); }
  INLINE const sseb operator > (const ssef& a, const ssef& b) { return _mm_cmpnle_ps(a.m128, b.m128); }
  INLINE const sseb operator >=(const ssef& a, const ssef& b) { return _mm_cmpnlt_ps(a.m128, b.m128); }

  INLINE const sseb operator ==(const ssef& a, const float b) { return _mm_cmpeq_ps (a.m128, ssef(b)); }
  INLINE const sseb operator !=(const ssef& a, const float b) { return _mm_cmpneq_ps(a.m128, ssef(b)); }
  INLINE const sseb operator < (const ssef& a, const float b) { return _mm_cmplt_ps (a.m128, ssef(b)); }
  INLINE const sseb operator <=(const ssef& a, const float b) { return _mm_cmple_ps (a.m128, ssef(b)); }
  INLINE const sseb operator > (const ssef& a, const float b) { return _mm_cmpnle_ps(a.m128, ssef(b)); }
  INLINE const sseb operator >=(const ssef& a, const float b) { return _mm_cmpnlt_ps(a.m128, ssef(b)); }

  INLINE const sseb operator ==(const float a, const ssef& b) { return _mm_cmpeq_ps (ssef(a), b.m128); }
  INLINE const sseb operator !=(const float a, const ssef& b) { return _mm_cmpneq_ps(ssef(a), b.m128); }
  INLINE const sseb operator < (const float a, const ssef& b) { return _mm_cmplt_ps (ssef(a), b.m128); }
  INLINE const sseb operator <=(const float a, const ssef& b) { return _mm_cmple_ps (ssef(a), b.m128); }
  INLINE const sseb operator > (const float a, const ssef& b) { return _mm_cmpnle_ps(ssef(a), b.m128); }
  INLINE const sseb operator >=(const float a, const ssef& b) { return _mm_cmpnlt_ps(ssef(a), b.m128); }

    /*! workaround for compiler bug in VS2008 */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
  INLINE const ssef select(const sseb& mask, const ssef& a, const ssef& b) { return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b)); }
#else
  INLINE const ssef select(const sseb& mask, const ssef& t, const ssef& f) { return _mm_blendv_ps(f, t, mask); }
#endif


  ////////////////////////////////////////////////////////////////////////////////
  /// Rounding Functions
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const ssef round_even(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }
  INLINE const ssef round_down(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF   ); }
  INLINE const ssef round_up  (const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF   ); }
  INLINE const ssef round_zero(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO      ); }

  INLINE const ssef floor(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF   ); }
  INLINE const ssef ceil (const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF   ); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////
  //INLINE const ssef shuffle8(const ssef& a, const ssei& shuf) { return _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(a), shuf)); }
  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3> INLINE const ssef shuffle(const ssef& b) {
    return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(index_3, index_2, index_1, index_0)));
  }
  //template<> INLINE const ssef shuffle<0, 0, 2, 2>(const ssef& b) { return _mm_moveldup_ps(b); }
  //template<> INLINE const ssef shuffle<1, 1, 3, 3>(const ssef& b) { return _mm_movehdup_ps(b); }
  //template<> INLINE const ssef shuffle<0, 1, 0, 1>(const ssef& b) { return _mm_castpd_ps(_mm_movedup_pd(_mm_castps_pd(b))); }
  template<size_t index> INLINE const ssef expand(const ssef& b) { return shuffle<index, index, index, index>(b); }
  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3> INLINE const ssef shuffle(const ssef& a, const ssef& b) {
    return _mm_shuffle_ps(a, b, _MM_SHUFFLE(index_3, index_2, index_1, index_0));
  }
  template<size_t dst, size_t src, size_t clr> INLINE const ssef insert(const ssef& a, const ssef& b) {
    return _mm_insert_ps(a, b, (dst << 4) | (src << 6) | clr);
  }
  template<size_t dst, size_t src> INLINE const ssef insert(const ssef& a, const ssef& b) { return insert<dst, src, 0>(a, b); }
  template<size_t dst> INLINE const ssef insert(const ssef& a, const float b) { return insert<dst, 0>(a, _mm_set_ss(b)); }
  template<size_t dst> INLINE const ssef inserti(const ssef& a, const int b) { return _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(a),b,3)); }
  template<size_t src> INLINE int extracti(const ssef& b) { return _mm_extract_ps(b, src); }
  template<size_t src> INLINE float extract(const ssef& b) { return extract<0>(expand<src>(b)); }
  template<> INLINE float extract<0>(const ssef& b) { return _mm_cvtss_f32(b); }

  INLINE ssef unpacklo(const ssef& a, const ssef& b) { return _mm_unpacklo_ps(a.m128, b.m128); }
  INLINE ssef unpackhi(const ssef& a, const ssef& b) { return _mm_unpackhi_ps(a.m128, b.m128); }

  INLINE void transpose(const ssef& r0, const ssef& r1, const ssef& r2, const ssef& r3, ssef& c0, ssef& c1, ssef& c2, ssef& c3) {
    ssef l02 = unpacklo(r0,r2);
    ssef h02 = unpackhi(r0,r2);
    ssef l13 = unpacklo(r1,r3);
    ssef h13 = unpackhi(r1,r3);
    c0 = unpacklo(l02,l13);
    c1 = unpackhi(l02,l13);
    c2 = unpacklo(h02,h13);
    c3 = unpackhi(h02,h13);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Reductions
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const ssef reduce_min(const ssef& v) { ssef h = min(shuffle<1,0,3,2>(v),v); return min(shuffle<2,3,0,1>(h),h); }
  INLINE const ssef reduce_max(const ssef& v) { ssef h = max(shuffle<1,0,3,2>(v),v); return max(shuffle<2,3,0,1>(h),h); }
  INLINE const ssef reduce_add(const ssef& v) { ssef h = shuffle<1,0,3,2>(v) + v; return shuffle<2,3,0,1>(h) + h; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  inline std::ostream& operator<<(std::ostream& cout, const ssef& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ">";
  }
}

#endif

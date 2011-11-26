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

#ifndef __PF_SSEF_HPP__
#define __PF_SSEF_HPP__

#include "sse.hpp"
#include "sse_swizzle.hpp"

namespace pf
{
  /*! 4-wide SSE float type. */
  struct ssef
  {
    typedef sseb Mask;
    __m128 m128;

    INLINE ssef(void) {}
    INLINE ssef(const ssef& other) { m128 = other.m128; }
    INLINE ssef(__m128 a) : m128(a) {}
    INLINE explicit ssef(__m128i a) : m128(_mm_cvtepi32_ps(a)) {}
    INLINE explicit ssef(const float *a) : m128(_mm_loadu_ps(a)) {}
    INLINE ssef(float a) : m128(_mm_set1_ps(a)) {}
    INLINE ssef(float a, float b, float c, float d) : m128(_mm_set_ps(d, c, b, a)) {}

    INLINE ssef& operator=(const ssef& other) { m128 = other.m128; return *this; }
    INLINE operator const __m128&(void) const { return m128; }
    INLINE operator       __m128&(void)       { return m128; }

    INLINE ssef(ZeroTy)   : m128(_mm_set1_ps(0.0f)) {}
    INLINE ssef(OneTy)    : m128(_mm_set1_ps(1.0f)) {}
    INLINE ssef(PosInfTy) : m128(_mm_set1_ps(pos_inf)) {}
    INLINE ssef(NegInfTy) : m128(_mm_set1_ps(neg_inf)) {}
    INLINE ssef(StepTy)   : m128(_mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f)) {}

    INLINE const float& operator[] (size_t index) const {
      assert(index < 4);
      return ((float*)&this->m128)[index];
    }
    INLINE float& operator[] (size_t index) {
      assert(index < 4);
      return ((float*)&this->m128)[index];
    }

    /*! All swizzle functions */
    template<size_t i0, size_t i1, size_t i2, size_t i3>
    INLINE const ssef swizzle(void) const {
      return _mm_castsi128_ps(
               _mm_shuffle_epi32(_mm_castps_si128(this->m128),
                 _MM_SHUFFLE(i3, i2, i1, i0)));
    }
    GEN_VEC4_SWZ_FUNCS(ssef);

    /*! Aligned load and store */
    static INLINE ssef load(const float *a) { return _mm_load_ps(a); }
    static INLINE void store(ssef x, float *a) { return _mm_store_ps(a, x.m128); }

    /*! Convenient constants */
    static INLINE uint32 laneNum(void) { return sizeof(ssef) / sizeof(float); }
    static INLINE ssef laneNumv(void)  { return LANE_NUM; }
    static INLINE ssef one(void)       { return ONE; }
    static INLINE ssef identity(void)  { return IDENTITY; }
    static INLINE ssef epsilon(void)   { return EPSILON; }
    static const uint32 CHANNEL_NUM = 4;

  private:
    static const ssef ONE;      //!< Stores {1,1,1,1}
    static const ssef IDENTITY; //!< Stores {0,1,2,3}
    static const ssef LANE_NUM; //!< Stores {4,4,4,4}
    static const ssef EPSILON;  //!< Stores {eps,eps,eps,eps}
  };

  INLINE const ssef operator+ (const ssef& a) { return a; }
  INLINE const ssef operator- (const ssef& a) {
    const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
    return _mm_xor_ps(a.m128, mask);
  }

  INLINE const ssef abs  (const ssef& a) {
    const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    return _mm_and_ps(a.m128, mask);
  }

  INLINE const ssef sign (const ssef& a) { return _mm_blendv_ps(ssef(one), -ssef(one), _mm_cmplt_ps (a,ssef(zero))); }
  INLINE const ssef rcp  (const ssef& a) { return _mm_div_ps(ssef::one(), a.m128); }
  INLINE const ssef sqrt (const ssef& a) { return _mm_sqrt_ps(a.m128); }
  INLINE const ssef sqr  (const ssef& a) { return _mm_mul_ps(a,a); }
  INLINE const ssef rsqrt(const ssef& a) {
    const ssef r = _mm_rsqrt_ps(a.m128);
    return _mm_add_ps(_mm_mul_ps(_mm_set_ps(1.5f, 1.5f, 1.5f, 1.5f), r),
                        _mm_mul_ps(_mm_mul_ps(
                          _mm_mul_ps(a, _mm_set_ps(-0.5f, -0.5f, -0.5f, -0.5f)), r),
                            _mm_mul_ps(r, r)));
  }

  INLINE ssef operator+ (const ssef& a, const ssef& b) { return _mm_add_ps(a.m128, b.m128); }
  INLINE ssef operator- (const ssef& a, const ssef& b) { return _mm_sub_ps(a.m128, b.m128); }
  INLINE ssef operator* (const ssef& a, const ssef& b) { return _mm_mul_ps(a.m128, b.m128); }
  INLINE ssef operator/ (const ssef& a, const ssef& b) { return _mm_div_ps(a.m128, b.m128); }
  INLINE ssef operator+ (const ssef& a, float b) { return a + ssef(b); }
  INLINE ssef operator- (const ssef& a, float b) { return a - ssef(b); }
  INLINE ssef operator* (const ssef& a, float b) { return a * ssef(b); }
  INLINE ssef operator/ (const ssef& a, float b) { return a / ssef(b); }
  INLINE ssef operator+ (float a, const ssef& b) { return ssef(a) + b; }
  INLINE ssef operator- (float a, const ssef& b) { return ssef(a) - b; }
  INLINE ssef operator* (float a, const ssef& b) { return ssef(a) * b; }
  INLINE ssef operator/ (float a, const ssef& b) { return ssef(a) / b; }
  INLINE ssef& operator+= (ssef& a, const ssef& b) { return a = a + b; }
  INLINE ssef& operator-= (ssef& a, const ssef& b) { return a = a - b; }
  INLINE ssef& operator*= (ssef& a, const ssef& b) { return a = a * b; }
  INLINE ssef& operator/= (ssef& a, const ssef& b) { return a = a / b; }
  INLINE ssef& operator+= (ssef& a, float b) { return a = a + b; }
  INLINE ssef& operator-= (ssef& a, float b) { return a = a - b; }
  INLINE ssef& operator*= (ssef& a, float b) { return a = a * b; }
  INLINE ssef& operator/= (ssef& a, float b) { return a = a / b; }

  INLINE ssef max(const ssef& a, const ssef& b) { return _mm_max_ps(a.m128,b.m128); }
  INLINE ssef min(const ssef& a, const ssef& b) { return _mm_min_ps(a.m128,b.m128); }
  INLINE ssef max(const ssef& a, float b) { return _mm_max_ps(a.m128,ssef(b)); }
  INLINE ssef min(const ssef& a, float b) { return _mm_min_ps(a.m128,ssef(b)); }
  INLINE ssef max(float a, const ssef& b) { return _mm_max_ps(ssef(a),b.m128); }
  INLINE ssef min(float a, const ssef& b) { return _mm_min_ps(ssef(a),b.m128); }
  INLINE ssef mulss(const ssef &a, const ssef &b) { return _mm_mul_ss(a,b); }
  INLINE ssef divss(const ssef &a, const ssef &b) { return _mm_div_ss(a,b); }
  INLINE ssef subss(const ssef &a, const ssef &b) { return _mm_sub_ss(a,b); }
  INLINE ssef addss(const ssef &a, const ssef &b) { return _mm_add_ss(a,b); }
  INLINE ssef mulss(const ssef &a, float b) { return _mm_mul_ss(a,ssef(b)); }
  INLINE ssef divss(const ssef &a, float b) { return _mm_div_ss(a,ssef(b)); }
  INLINE ssef subss(const ssef &a, float b) { return _mm_sub_ss(a,ssef(b)); }
  INLINE ssef addss(const ssef &a, float b) { return _mm_add_ss(a,ssef(b)); }
  INLINE ssef mulss(float a, const ssef &b) { return _mm_mul_ss(ssef(a),b); }
  INLINE ssef divss(float a, const ssef &b) { return _mm_div_ss(ssef(a),b); }
  INLINE ssef subss(float a, const ssef &b) { return _mm_sub_ss(ssef(a),b); }
  INLINE ssef addss(float a, const ssef &b) { return _mm_add_ss(ssef(a),b); }

  INLINE ssef operator^ (const ssef& a, const ssei& b) { return _mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(a.m128),b.m128)); }
  INLINE ssef operator^ (const ssef& a, const ssef& b) { return _mm_xor_ps(a.m128,b.m128); }
  INLINE ssef operator& (const ssef& a, const ssef& b) { return _mm_and_ps(a.m128,b.m128); }
  INLINE ssef operator& (const ssef& a, const ssei& b) { return _mm_and_ps(a.m128,_mm_castsi128_ps(b.m128)); }
  INLINE ssef operator| (const ssef& a, const ssef& b) { return _mm_or_ps(a.m128,b.m128); }
  INLINE ssef operator| (const ssef& a, const ssei& b) { return _mm_or_ps(a.m128,_mm_castsi128_ps(b.m128)); }

  INLINE sseb operator== (const ssef& a, const ssef& b) { return _mm_cmpeq_ps (a.m128, b.m128); }
  INLINE sseb operator!= (const ssef& a, const ssef& b) { return _mm_cmpneq_ps(a.m128, b.m128); }
  INLINE sseb operator<  (const ssef& a, const ssef& b) { return _mm_cmplt_ps (a.m128, b.m128); }
  INLINE sseb operator<= (const ssef& a, const ssef& b) { return _mm_cmple_ps (a.m128, b.m128); }
  INLINE sseb operator>  (const ssef& a, const ssef& b) { return _mm_cmpnle_ps(a.m128, b.m128); }
  INLINE sseb operator>= (const ssef& a, const ssef& b) { return _mm_cmpnlt_ps(a.m128, b.m128); }
  INLINE sseb operator== (const ssef& a, float b) { return _mm_cmpeq_ps (a.m128, ssef(b)); }
  INLINE sseb operator!= (const ssef& a, float b) { return _mm_cmpneq_ps(a.m128, ssef(b)); }
  INLINE sseb operator<  (const ssef& a, float b) { return _mm_cmplt_ps (a.m128, ssef(b)); }
  INLINE sseb operator<= (const ssef& a, float b) { return _mm_cmple_ps (a.m128, ssef(b)); }
  INLINE sseb operator>  (const ssef& a, float b) { return _mm_cmpnle_ps(a.m128, ssef(b)); }
  INLINE sseb operator>= (const ssef& a, float b) { return _mm_cmpnlt_ps(a.m128, ssef(b)); }
  INLINE sseb operator== (float a, const ssef& b) { return _mm_cmpeq_ps (ssef(a), b.m128); }
  INLINE sseb operator!= (float a, const ssef& b) { return _mm_cmpneq_ps(ssef(a), b.m128); }
  INLINE sseb operator<  (float a, const ssef& b) { return _mm_cmplt_ps (ssef(a), b.m128); }
  INLINE sseb operator<= (float a, const ssef& b) { return _mm_cmple_ps (ssef(a), b.m128); }
  INLINE sseb operator>  (float a, const ssef& b) { return _mm_cmpnle_ps(ssef(a), b.m128); }
  INLINE sseb operator>= (float a, const ssef& b) { return _mm_cmpnlt_ps(ssef(a), b.m128); }

    /*! workaround for compiler bug in VS2008 */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
  INLINE ssef select(const sseb& mask, const ssef& a, const ssef& b) { return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b)); }
#else
  INLINE ssef select(const sseb& mask, const ssef& t, const ssef& f) { return _mm_blendv_ps(f, t, mask); }
#endif /* defined(_MSC_VER) && (_MSC_VER < 1600) */

  INLINE ssef round_even(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }
  INLINE ssef round_down(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF); }
  INLINE ssef round_up  (const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF); }
  INLINE ssef round_zero(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO); }
  INLINE ssef floor(const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF); }
  INLINE ssef ceil (const ssef& a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF); }
  INLINE size_t movemask(const ssef& a) { return _mm_movemask_ps(a); }
  INLINE ssef fixup (const ssef& a) { return max(abs(a), ssef::epsilon()) ^ (a & ssei(0x80000000)); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////
  //INLINE const ssef shuffle8(const ssef& a, const ssei& shuf) { return _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(a), shuf)); }
  template<size_t index0, size_t index1, size_t index2, size_t index3>
  INLINE const ssef shuffle(const ssef& b) {
    return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(index3, index2, index1, index0)));
  }
  //template<> INLINE const ssef shuffle<0, 0, 2, 2>(const ssef& b) { return _mm_moveldup_ps(b); }
  //template<> INLINE const ssef shuffle<1, 1, 3, 3>(const ssef& b) { return _mm_movehdup_ps(b); }
  //template<> INLINE const ssef shuffle<0, 1, 0, 1>(const ssef& b) { return _mm_castpd_ps(_mm_movedup_pd(_mm_castps_pd(b))); }
  template<size_t index> INLINE const ssef expand(const ssef& b) { return shuffle<index, index, index, index>(b); }
  template<size_t i0, size_t i1, size_t i2, size_t i3>
  INLINE ssef shuffle(const ssef& a, const ssef& b) {
    return _mm_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
  }
  template<size_t dst, size_t src, size_t clr>
  INLINE ssef insert(const ssef& a, const ssef& b) {
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

  INLINE void transpose(const ssef& r0, const ssef& r1, const ssef& r2, const ssef& r3,
                              ssef& c0,       ssef& c1,       ssef& c2,       ssef& c3)
  {
    ssef l02 = unpacklo(r0,r2);
    ssef h02 = unpackhi(r0,r2);
    ssef l13 = unpacklo(r1,r3);
    ssef h13 = unpackhi(r1,r3);
    c0 = unpacklo(l02,l13);
    c1 = unpackhi(l02,l13);
    c2 = unpacklo(h02,h13);
    c3 = unpackhi(h02,h13);
  }

  INLINE ssef reduce_min(const ssef& v) { ssef h = min(shuffle<1,0,3,2>(v),v); return min(shuffle<2,3,0,1>(h),h); }
  INLINE ssef reduce_max(const ssef& v) { ssef h = max(shuffle<1,0,3,2>(v),v); return max(shuffle<2,3,0,1>(h),h); }
  INLINE ssef reduce_add(const ssef& v) { ssef h = shuffle<1,0,3,2>(v) + v; return shuffle<2,3,0,1>(h) + h; }

  inline std::ostream& operator<<(std::ostream& cout, const ssef& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ">";
  }
}

#endif /* __PF_SSEF_HPP__ */


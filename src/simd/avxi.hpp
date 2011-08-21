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

#ifndef __PF_AVXI_H__
#define __PF_AVXI_H__

namespace pf
{
  /*! 8-wide AVX integer type. Implements an 8 wide integer type using
   *  AVX. As AVX does not support integers, we implement some
   *  operations using floating point. Thus use with care! */

  struct avxi
  {
    union { __m256i m256; int32 v[8]; };

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////

    typedef avxb Mask;

    INLINE avxi           ( ) {}
    INLINE avxi           ( const avxi& other ) { m256 = other.m256; }
    INLINE avxi& operator=( const avxi& other ) { m256 = other.m256; return *this; }

    INLINE avxi( const __m256i a ) : m256(a) {}
    INLINE explicit avxi( const __m256 a ) : m256(_mm256_cvtps_epi32(a)) {}

    INLINE explicit avxi( const int* const a ) : m256(_mm256_castps_si256(_mm256_loadu_ps((const float*)a))) {}
    INLINE          avxi( const int&       a ) : m256(_mm256_castps_si256(_mm256_broadcast_ss((const float*)&a))) {}

    INLINE explicit avxi( const ssei& a ) : m256(_mm256_insertf128_si256(_mm256_castsi128_si256(a),a,1)) {}
    INLINE avxi( const ssei& a, const ssei& b ) : m256(_mm256_insertf128_si256(_mm256_castsi128_si256(a),b,1)) {}
    INLINE avxi( int a, int b, int c, int d, int e, int f, int g, int h ) : m256(_mm256_set_epi32(h, g, f, e, d, c, b, a)) {}

    INLINE operator const __m256i&( void ) const { return m256; }
    INLINE operator       __m256i&( void )       { return m256; }
    INLINE operator          ssei ( void ) const { return ssei( _mm256_castsi256_si128(m256)); }


    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    INLINE avxi( ZeroTy   ) : m256(_mm256_set1_epi32(0)) {}
    INLINE avxi( OneTy    ) : m256(_mm256_set1_epi32(1)) {}
    INLINE avxi( PosInfTy ) : m256(_mm256_set1_epi32(pos_inf)) {}
    INLINE avxi( NegInfTy ) : m256(_mm256_set1_epi32(neg_inf)) {}
    INLINE avxi( StepTy   ) : m256(_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0)) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// Properties
    ////////////////////////////////////////////////////////////////////////////////

    INLINE const int32& operator []( const size_t index ) const { assert(index < 8); return v[index]; }
    INLINE       int32& operator []( const size_t index )       { assert(index < 8); return v[index]; }
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxi operator +( const avxi& a ) { return a; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxi operator +( const avxi& a, const avxi& b ) { return _mm256_cvtps_epi32(_mm256_add_ps(_mm256_cvtepi32_ps(a), _mm256_cvtepi32_ps(b))); }
  INLINE const avxi operator -( const avxi& a, const avxi& b ) { return _mm256_cvtps_epi32(_mm256_sub_ps(_mm256_cvtepi32_ps(a), _mm256_cvtepi32_ps(b))); }
  INLINE const avxi operator *( const avxi& a, const avxi& b ) { return _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_cvtepi32_ps(a), _mm256_cvtepi32_ps(b))); }

  INLINE const avxi operator  &( const avxi& a, const avxi& b ) { return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
  INLINE const avxi operator  |( const avxi& a, const avxi& b ) { return _mm256_castps_si256(_mm256_or_ps (_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
  INLINE const avxi operator  ^( const avxi& a, const avxi& b ) { return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }

  INLINE const avxi operator  &( const avxi& a, const int32 b ) { return a & avxi(b); }
  INLINE const avxi operator  |( const avxi& a, const int32 b ) { return a | avxi(b); }
  INLINE const avxi operator  ^( const avxi& a, const int32 b ) { return a ^ avxi(b); }

  INLINE avxi& operator  &=( avxi& a, const avxi& b ) { return a = a  & b; }
  INLINE avxi& operator  |=( avxi& a, const avxi& b ) { return a = a  | b; }
  INLINE avxi& operator  ^=( avxi& a, const avxi& b ) { return a = a  ^ b; }

  INLINE const avxi min( const avxi& a, const avxi& b ) { return _mm256_cvtps_epi32(_mm256_min_ps(_mm256_cvtepi32_ps(a), _mm256_cvtepi32_ps(b))); }
  INLINE const avxi max( const avxi& a, const avxi& b ) { return _mm256_cvtps_epi32(_mm256_max_ps(_mm256_cvtepi32_ps(a), _mm256_cvtepi32_ps(b))); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxb operator ==( const avxi& a, const avxi& b ) { return _mm256_cmp_ps(_mm256_cvtepi32_ps(a.m256), _mm256_cvtepi32_ps(b.m256), _CMP_EQ_UQ ); }
  INLINE const avxb operator < ( const avxi& a, const avxi& b ) { return _mm256_cmp_ps(_mm256_cvtepi32_ps(a.m256), _mm256_cvtepi32_ps(b.m256), _CMP_NGE_UQ ); }
  INLINE const avxb operator <=( const avxi& a, const avxi& b ) { return _mm256_cmp_ps(_mm256_cvtepi32_ps(a.m256), _mm256_cvtepi32_ps(b.m256), _CMP_NGT_UQ ); }
  INLINE const avxb operator !=( const avxi& a, const avxi& b ) { return _mm256_cmp_ps(_mm256_cvtepi32_ps(a.m256), _mm256_cvtepi32_ps(b.m256), _CMP_NEQ_UQ); }
  INLINE const avxb operator >=( const avxi& a, const avxi& b ) { return _mm256_cmp_ps(_mm256_cvtepi32_ps(a.m256), _mm256_cvtepi32_ps(b.m256), _CMP_NLT_UQ); }
  INLINE const avxb operator > ( const avxi& a, const avxi& b ) { return _mm256_cmp_ps(_mm256_cvtepi32_ps(a.m256), _mm256_cvtepi32_ps(b.m256), _CMP_NLE_UQ); }

  INLINE const avxi select( const avxb& mask, const avxi& t, const avxi& f ) { return _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(f), _mm256_castsi256_ps(t), mask)); }


  ////////////////////////////////////////////////////////////////////////////////
  /// Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxi broadcast(const int* ptr) { return _mm256_castps_si256(_mm256_broadcast_ss((const float*)ptr)); }
  template<index_t index> INLINE const avxi insert (const avxi& a, const ssei& b) { return _mm256_insertf128_si256 (a,b,index); }
  template<index_t index> INLINE const ssei extract(const avxi& a               ) { return _mm256_extractf128_si256(a  ,index); }

  INLINE avxi unpacklo( const avxi& a, const avxi& b ) { return _mm256_castps_si256(_mm256_unpacklo_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
  INLINE avxi unpackhi( const avxi& a, const avxi& b ) { return _mm256_castps_si256(_mm256_unpackhi_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }

  template<index_t index> INLINE const avxi shuffle( const avxi& a ) {
    return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(a), _MM_SHUFFLE(index, index, index, index)));
  }

  template<index_t index_0, index_t index_1> INLINE const avxi shuffle( const avxi& a ) {
    return _mm256_permute2f128_si256(a, a, (index_1 << 4) | (index_0 << 0));
  }

  template<index_t index_0, index_t index_1> INLINE const avxi shuffle( const avxi& a,  const avxi& b) {
    return _mm256_permute2f128_si256(a, b, (index_1 << 4) | (index_0 << 0));
  }

  template<index_t index_0, index_t index_1, index_t index_2, index_t index_3> INLINE const avxi shuffle( const avxi& a ) {
    return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(a), _MM_SHUFFLE(index_3, index_2, index_1, index_0)));
  }

  template<index_t index_0, index_t index_1, index_t index_2, index_t index_3> INLINE const avxi shuffle( const avxi& a, const avxi& b ) {
    return _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b), _MM_SHUFFLE(index_3, index_2, index_1, index_0)));
  }

  template<> INLINE const avxi shuffle<0, 0, 2, 2>( const avxi& b ) { return _mm256_castps_si256(_mm256_moveldup_ps(_mm256_castsi256_ps(b))); }
  template<> INLINE const avxi shuffle<1, 1, 3, 3>( const avxi& b ) { return _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b))); }
  template<> INLINE const avxi shuffle<0, 1, 0, 1>( const avxi& b ) { return _mm256_castps_si256(_mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(_mm256_castsi256_ps(b))))); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  inline std::ostream& operator<<(std::ostream& cout, const avxi& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ", " << a[4] << ", " << a[5] << ", " << a[6] << ", " << a[7] << ">";
  }
}

#endif

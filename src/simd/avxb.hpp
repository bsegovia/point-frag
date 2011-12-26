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

#ifndef __PF_AVXB_HPP__
#define __PF_AVXB_HPP__

#include "sys/platform.hpp"

namespace pf
{
  /*! 8-wide AVX bool type. */
  struct avxb
  {
    union { __m256 m256; int32 v[8]; };

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////

    INLINE avxb           () {}
    INLINE avxb           ( const avxb& other ) { m256 = other.m256; }
    INLINE avxb& operator=( const avxb& other ) { m256 = other.m256; return *this; }

    INLINE avxb( const __m256  input ) : m256(input) {}
    INLINE avxb( const __m256i input ) : m256(_mm256_castsi256_ps(input)) {}
    INLINE avxb( const __m256d input ) : m256(_mm256_castpd_ps(input)) {}

    INLINE avxb( const bool& a ) : m256(a ? _mm256_cmp_ps(_mm256_setzero_ps(), _mm256_setzero_ps(), _CMP_EQ_OQ) : _mm256_setzero_ps()) {}

    INLINE avxb( const sseb& a ) : m256(_mm256_insertf128_ps(_mm256_castps128_ps256(a),a,1)) {}
    INLINE avxb( const sseb& a, const sseb& b ) : m256(_mm256_insertf128_ps(_mm256_castps128_ps256(a),b,1)) {}

    INLINE operator const __m256&( void ) const { return m256; }
    INLINE operator const __m256i( void ) const { return _mm256_castps_si256(m256); }
    INLINE operator const __m256d( void ) const { return _mm256_castps_pd(m256); }
    INLINE operator         sseb ( void ) const { return sseb(_mm256_castps256_ps128(m256)); }

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    INLINE avxb( FalseTy ) : m256(_mm256_setzero_ps()) {}
    INLINE avxb( TrueTy  ) : m256(_mm256_cmp_ps(_mm256_setzero_ps(), _mm256_setzero_ps(), _CMP_EQ_OQ)) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// Properties
    ////////////////////////////////////////////////////////////////////////////////

    INLINE bool   operator []( const size_t index ) const { PF_ASSERT(index < 8); return (_mm256_movemask_ps(m256) >> index) & 1; }
    INLINE int32& operator []( const size_t index )       { PF_ASSERT(index < 8); return v[index]; }
    PF_STRUCT(avxb);
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxb operator !( const avxb& a ) { return _mm256_xor_ps(a, avxb(True)); }


  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxb operator &( const avxb& a, const avxb& b ) { return _mm256_and_ps(a, b); }
  INLINE const avxb operator |( const avxb& a, const avxb& b ) { return _mm256_or_ps (a, b); }
  INLINE const avxb operator ^( const avxb& a, const avxb& b ) { return _mm256_xor_ps(a, b); }

  INLINE avxb operator &=( avxb& a, const avxb& b ) { return a = a & b; }
  INLINE avxb operator |=( avxb& a, const avxb& b ) { return a = a | b; }
  INLINE avxb operator ^=( avxb& a, const avxb& b ) { return a = a ^ b; }


  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

  INLINE const avxb operator !=( const avxb& a, const avxb& b ) { return _mm256_xor_ps(a, b); }
  INLINE const avxb operator ==( const avxb& a, const avxb& b ) { return _mm256_xor_ps(_mm256_xor_ps(a,b),avxb(True)); }

  INLINE const avxb select( const avxb& mask, const avxb& t, const avxb& f ) { return _mm256_blendv_ps(f, t, mask); }


  ////////////////////////////////////////////////////////////////////////////////
  /// Reduction Operations
  ////////////////////////////////////////////////////////////////////////////////

  INLINE bool reduce_and( const avxb& a ) { return _mm256_movemask_ps(a) == 0xff; }
  INLINE bool reduce_or ( const avxb& a ) { return !_mm256_testz_ps(a,a); }

  INLINE bool all       ( const avxb& a ) { return _mm256_movemask_ps(a) == 0xff; }
  INLINE bool none      ( const avxb& a ) { return _mm256_testz_ps(a,a) != 0; }
  INLINE bool any       ( const avxb& a ) { return !_mm256_testz_ps(a,a); }

  INLINE size_t movemask( const avxb& a ) { return _mm256_movemask_ps(a); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////

  template<size_t index> INLINE const avxb insert (const avxb& a, const sseb& b) { return _mm256_insertf128_ps (a,b,index); }
  template<size_t index> INLINE const sseb extract(const avxb& a               ) { return _mm256_extractf128_ps(a  ,index); }

  INLINE avxb unpacklo( const avxb& a, const avxb& b ) { return _mm256_unpacklo_ps(a.m256, b.m256); }
  INLINE avxb unpackhi( const avxb& a, const avxb& b ) { return _mm256_unpackhi_ps(a.m256, b.m256); }

  template<size_t index> INLINE const avxb shuffle( const avxb& a ) {
    return _mm256_permute_ps(a, _MM_SHUFFLE(index, index, index, index));
  }

  template<size_t index_0, size_t index_1> INLINE const avxb shuffle( const avxb& a ) {
    return _mm256_permute2f128_ps(a, a, (index_1 << 4) | (index_0 << 0));
  }

  template<size_t index_0, size_t index_1> INLINE const avxb shuffle( const avxb& a,  const avxb& b) {
    return _mm256_permute2f128_ps(a, b, (index_1 << 4) | (index_0 << 0));
  }

  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3> INLINE const avxb shuffle( const avxb& a ) {
    return _mm256_permute_ps(a, _MM_SHUFFLE(index_3, index_2, index_1, index_0));
  }

  template<size_t index_0, size_t index_1, size_t index_2, size_t index_3> INLINE const avxb shuffle( const avxb& a, const avxb& b ) {
    return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(index_3, index_2, index_1, index_0));
  }

  template<> INLINE const avxb shuffle<0, 0, 2, 2>( const avxb& b ) { return _mm256_moveldup_ps(b); }
  template<> INLINE const avxb shuffle<1, 1, 3, 3>( const avxb& b ) { return _mm256_movehdup_ps(b); }
  template<> INLINE const avxb shuffle<0, 1, 0, 1>( const avxb& b ) { return _mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(b))); }


  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  inline std::ostream& operator<<(std::ostream& cout, const avxb& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ", "
                       << a[4] << ", " << a[5] << ", " << a[6] << ", " << a[7] << ">";
  }
}

#endif /* __PF_AVXB_HPP__ */


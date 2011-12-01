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

#ifndef __PF_CONSTANTS_H__
#define __PF_CONSTANTS_H__

#ifndef NULL
#define NULL 0
#endif

#include <limits>

namespace pf
{
  static struct NullTy {
  } null MAYBE_UNUSED;

  static struct TrueTy {
    INLINE operator bool( ) const { return true; }
  } True MAYBE_UNUSED;

  static struct FalseTy {
    INLINE operator bool( ) const { return false; }
  } False MAYBE_UNUSED;

  static struct ZeroTy
  {
    INLINE operator double( ) const { return 0; }
    INLINE operator float ( ) const { return 0; }
    INLINE operator int64 ( ) const { return 0; }
    INLINE operator uint64( ) const { return 0; }
    INLINE operator int32 ( ) const { return 0; }
    INLINE operator uint32( ) const { return 0; }
    INLINE operator int16 ( ) const { return 0; }
    INLINE operator uint16( ) const { return 0; }
    INLINE operator int8  ( ) const { return 0; }
    INLINE operator uint8 ( ) const { return 0; }
#ifndef __MSVC__
    INLINE operator size_t( ) const { return 0; }
#endif

  } zero MAYBE_UNUSED;

  static struct OneTy
  {
    INLINE operator double( ) const { return 1; }
    INLINE operator float ( ) const { return 1; }
    INLINE operator int64 ( ) const { return 1; }
    INLINE operator uint64( ) const { return 1; }
    INLINE operator int32 ( ) const { return 1; }
    INLINE operator uint32( ) const { return 1; }
    INLINE operator int16 ( ) const { return 1; }
    INLINE operator uint16( ) const { return 1; }
    INLINE operator int8  ( ) const { return 1; }
    INLINE operator uint8 ( ) const { return 1; }
#ifndef __MSVC__
    INLINE operator size_t( ) const { return 1; }
#endif
  } one MAYBE_UNUSED;

  static struct NegInfTy
  {
    INLINE operator double( ) const { return -std::numeric_limits<double>::infinity(); }
    INLINE operator float ( ) const { return -std::numeric_limits<float>::infinity(); }
    INLINE operator int64 ( ) const { return std::numeric_limits<int64>::min(); }
    INLINE operator uint64( ) const { return std::numeric_limits<uint64>::min(); }
    INLINE operator int32 ( ) const { return std::numeric_limits<int32>::min(); }
    INLINE operator uint32( ) const { return std::numeric_limits<uint32>::min(); }
    INLINE operator int16 ( ) const { return std::numeric_limits<int16>::min(); }
    INLINE operator uint16( ) const { return std::numeric_limits<uint16>::min(); }
    INLINE operator int8  ( ) const { return std::numeric_limits<int8>::min(); }
    INLINE operator uint8 ( ) const { return std::numeric_limits<uint8>::min(); }
#ifndef __MSVC__
    INLINE operator size_t( ) const { return std::numeric_limits<size_t>::min(); }
#endif

  } neg_inf MAYBE_UNUSED;

  static struct PosInfTy
  {
    INLINE operator double( ) const { return std::numeric_limits<double>::infinity(); }
    INLINE operator float ( ) const { return std::numeric_limits<float>::infinity(); }
    INLINE operator int64 ( ) const { return std::numeric_limits<int64>::max(); }
    INLINE operator uint64( ) const { return std::numeric_limits<uint64>::max(); }
    INLINE operator int32 ( ) const { return std::numeric_limits<int32>::max(); }
    INLINE operator uint32( ) const { return std::numeric_limits<uint32>::max(); }
    INLINE operator int16 ( ) const { return std::numeric_limits<int16>::max(); }
    INLINE operator uint16( ) const { return std::numeric_limits<uint16>::max(); }
    INLINE operator int8  ( ) const { return std::numeric_limits<int8>::max(); }
    INLINE operator uint8 ( ) const { return std::numeric_limits<uint8>::max(); }
#ifndef _WIN32
    INLINE operator size_t( ) const { return std::numeric_limits<size_t>::max(); }
#endif
  } inf MAYBE_UNUSED, pos_inf MAYBE_UNUSED;

  static struct NaNTy
  {
    INLINE operator double( ) const { return std::numeric_limits<double>::quiet_NaN(); }
    INLINE operator float ( ) const { return std::numeric_limits<float>::quiet_NaN(); }
  } nan MAYBE_UNUSED;

  static struct UlpTy
  {
    INLINE operator double( ) const { return std::numeric_limits<double>::epsilon(); }
    INLINE operator float ( ) const { return std::numeric_limits<float>::epsilon(); }
  } ulp MAYBE_UNUSED;

  static struct PiTy
  {
    INLINE operator double( ) const { return 3.14159265358979323846; }
    INLINE operator float ( ) const { return 3.14159265358979323846f; }
  } pi MAYBE_UNUSED;

  static struct StepTy {
  } step MAYBE_UNUSED;

  static struct EmptyTy {
  } empty MAYBE_UNUSED;

  static struct FullTy {
  } full MAYBE_UNUSED;

  static const size_t KB = 1024u;
  static const size_t MB = KB*KB;
  static const size_t GB = KB*MB;
}

#endif

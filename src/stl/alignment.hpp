// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STL_ALIGNMENT_HPP__
#define __PF_STL_ALIGNMENT_HPP__

#include "rdestl_common.hpp"
#include <cstddef>
#include <xmmintrin.h>

namespace pf
{
  namespace internal
  {
#if defined(__MSVC__) || defined(__ICC__)
#pragma warning(push)
#pragma warning(disable: 4324)
    template<typename T>
    struct alignof_helper
    {
      char  x;
      T     y;
    };
#pragma warning(pop)
#endif /* __MSVC__ */
    template<size_t N> struct type_with_alignment
    {
      typedef char err_invalid_alignment[N > 0 ? -1 : 1];
    };
    template<> struct type_with_alignment<0>  {};
    template<> struct type_with_alignment<1>  { uint8 member; };
    template<> struct type_with_alignment<2>  { uint16 member; };
    template<> struct type_with_alignment<4>  { uint32 member; };
    template<> struct type_with_alignment<8>  { uint64 member; };
    template<> struct type_with_alignment<16> { __m128 member; };
  } /* namespace internal */
#if defined(__MSVC__) || defined(__ICC__)
  template<typename T>
  struct alignof
  {
    static const uint32 res =  offsetof(internal::alignof_helper<T>, y);
  };
  template<typename T>
  struct aligned_as
  {
    typedef typename internal::type_with_alignment<alignof<T>::res> res;
  };
#else
  template<typename T>
  struct aligned_as
  {
    typedef typename internal::type_with_alignment<alignof(T)> res;
  };
#endif /* __MSVC__ */

} /* namespace pf */

#endif /* __PF_STL_ALIGNMENT_HPP__ */


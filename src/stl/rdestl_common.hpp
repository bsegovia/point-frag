// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STL_COMMON_HPP__
#define __PF_STL_COMMON_HPP__

#define RDESTL_STANDALONE  1

#include "sys/platform.hpp"
#include <cstring>

namespace pf
{
  namespace Sys
  {
    INLINE void MemCpy(void* to, const void* from, size_t bytes) { memcpy(to, from, bytes); }
    INLINE void MemMove(void* to, const void* from, size_t bytes) { memmove(to, from, bytes); }
    INLINE void MemSet(void* buf, unsigned char value, size_t bytes) { memset(buf, value, bytes); }
  } /* namespace Sys */
  enum e_noinitialize { noinitialize };
} /* namespace pf */

#endif /* __PF_STL_COMMON_HPP__ */


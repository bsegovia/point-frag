#ifndef __PF_ALLOC_HPP__
#define __PF_ALLOC_HPP__

#include <cstdlib>

namespace pf
{
  /*! regular allocation (counted) */
  void* malloc(size_t size);
  void* realloc(void *ptr, size_t size);
  void  free(void *ptr);

  /*! aligned malloc */
  void* alignedMalloc(size_t size, size_t align = 64);
  void  alignedFree(void* ptr);
}

#endif /* __PF_ALLOC_HPP__ */


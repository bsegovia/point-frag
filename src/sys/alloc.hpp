#ifndef __PF_ALLOC_HPP__
#define __PF_ALLOC_HPP__

#include <cstdlib>
#include "sys/platform.hpp"

namespace pf
{
  /*! regular allocation (counted if memory debugger is used) */
  void* malloc(size_t size);
  void* realloc(void *ptr, size_t size);
  void  free(void *ptr);

  /*! aligned malloc (counted if memory debugger is used) */
  void* alignedMalloc(size_t size, size_t align = 64);
  void  alignedFree(void* ptr);

  /*! Used to count memory allocations */
#if DEBUG_MEMORY
  void* insertAlloc(void*, const char*, const char*, int);
  void  removeAlloc(void *ptr);
  void  dumpAlloc(void);
  void  startMemoryDebugger(void);
  void  endMemoryDebugger(void);
#else
  INLINE void* insertAlloc(void *ptr, const char*, const char*, int) {return ptr;}
  INLINE void  removeAlloc(void *ptr) {}
  INLINE void  dumpAlloc(void) {}
  INLINE void  startMemoryDebugger(void) {}
  INLINE void  endMemoryDebugger(void) {}
#endif /* DEBUG_MEMORY */

  /*! Properly handle the allocated type */
  template <typename T>
  T* _insertAlloc(T *ptr, const char *file, const char *function, int line) {
    insertAlloc(ptr, file, function, line);
    return ptr;
  }

  /*! Macros to handle allocation position */
#define NEW(T,...)         _insertAlloc(new T(__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)
#define NEW_ARRAY(T,N,...) _insertAlloc(new T[N](__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)
#define DELETE(X)          do { removeAlloc(X); delete X; } while (0)
#define DELETE_ARRAY(X)    do { removeAlloc(X); delete[] X; } while (0)
}

#endif /* __PF_ALLOC_HPP__ */


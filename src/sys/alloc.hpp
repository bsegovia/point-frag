// ======================================================================== //
// Copyright (C) 2011 Benjamin Segovia                                      //
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

#ifndef __PF_ALLOC_HPP__
#define __PF_ALLOC_HPP__

#include "sys/platform.hpp"
#include <cstdlib>

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
#if PF_DEBUG_MEMORY
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
#endif /* PF_DEBUG_MEMORY */

  /*! Properly handle the allocated type */
  template <typename T>
  T* _insertAlloc(T *ptr, const char *file, const char *function, int line) {
    insertAlloc(ptr, file, function, line);
    return ptr;
  }
}

/*! Macros to handle allocation position */
#define PF_NEW(T,...)         _insertAlloc(new T(__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)
#define PF_NEW_ARRAY(T,N,...) _insertAlloc(new T[N](__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)
#define PF_DELETE(X)          do { removeAlloc(X); delete X; } while (0)
#define PF_DELETE_ARRAY(X)    do { removeAlloc(X); delete[] X; } while (0)
#define PF_MALLOC(SZ)         insertAlloc(malloc(SZ),__FILE__, __FUNCTION__, __LINE__)
#define PF_REALLOC(X,SZ)      do { assert(0); FATAL("unsupported macro"); } while (0)
#define PF_FREE(X)            do { removeAlloc(X); free(X); } while (0)
#define PF_ALIGNED_FREE(X)    do { removeAlloc(X); alignedFree(X); } while (0)
#define PF_ALIGNED_MALLOC(SZ,ALIGN) insertAlloc(alignedMalloc(SZ,ALIGN),__FILE__, __FUNCTION__, __LINE__)

#endif /* __PF_ALLOC_HPP__ */


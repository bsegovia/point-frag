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
  /*! regular allocation */
  void* malloc(size_t size);
  void* realloc(void *ptr, size_t size);
  void  free(void *ptr);

  /*! aligned malloc */
  void* alignedMalloc(size_t size, size_t align = 64);
  void  alignedFree(void* ptr);

  /*! Monitor memory allocations */
#if PF_DEBUG_MEMORY
  void* MemDebuggerInsertAlloc(void*, const char*, const char*, int);
  void  MemDebuggerRemoveAlloc(void *ptr);
  void  MemDebuggerDumpAlloc(void);
  void  MemDebuggerStart(void);
  void  MemDebuggerEnd(void);
#else
  INLINE void* MemDebuggerInsertAlloc(void *ptr, const char*, const char*, int) {return ptr;}
  INLINE void  MemDebuggerRemoveAlloc(void *ptr) {}
  INLINE void  MemDebuggerDumpAlloc(void) {}
  INLINE void  MemDebuggerStart(void) {}
  INLINE void  MemDebuggerEnd(void) {}
#endif /* PF_DEBUG_MEMORY */

  /*! Properly handle the allocated type */
  template <typename T>
  T* _MemDebuggerInsertAlloc(T *ptr, const char *file, const char *function, int line) {
    MemDebuggerInsertAlloc(ptr, file, function, line);
    return ptr;
  }
} /* namespace pf */

/*! Macros to handle allocation position */
#define PF_NEW(T,...)               \
  pf::_MemDebuggerInsertAlloc(new T(__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)

#define PF_NEW_ARRAY(T,N,...)       \
  pf::_MemDebuggerInsertAlloc(new T[N](__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)

#define PF_NEW_P(T,X,...)           \
  pf::_MemDebuggerInsertAlloc(new (X) T(__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)

#define PF_DELETE(X)                \
  do { pf::MemDebuggerRemoveAlloc(X); delete X; } while (0)

#define PF_DELETE_ARRAY(X)          \
  do { pf::MemDebuggerRemoveAlloc(X); delete[] X; } while (0)

#define PF_MALLOC(SZ)               \
  pf::MemDebuggerInsertAlloc(pf::malloc(SZ),__FILE__, __FUNCTION__, __LINE__)

#define PF_REALLOC(PTR, SZ)         \
  pf::MemDebuggerInsertAlloc(pf::realloc(PTR, SZ),__FILE__, __FUNCTION__, __LINE__)

#define PF_FREE(X)                  \
  do { pf::MemDebuggerRemoveAlloc(X); pf::free(X); } while (0)

#define PF_ALIGNED_FREE(X)          \
  do { pf::MemDebuggerRemoveAlloc(X); pf::alignedFree(X); } while (0)

#define PF_ALIGNED_MALLOC(SZ,ALIGN) \
  pf::MemDebuggerInsertAlloc(pf::alignedMalloc(SZ,ALIGN),__FILE__, __FUNCTION__, __LINE__)

namespace pf
{
  /*! A growing pool never deallocates */
  template <typename T>
  class GrowingPool
  {
  public:
    GrowingPool(void) : current(PF_NEW(GrowingPoolElem, 1)) {}
    ~GrowingPool(void) { assert(current); PF_DELETE(current); }
    T *allocate(void) {
      if (UNLIKELY(current->allocated == current->maxElemNum)) {
        GrowingPoolElem *elem = PF_NEW(GrowingPoolElem, 2 * current->maxElemNum);
        elem->next = current;
        current = elem;
      }
      T *data = current->data + current->allocated++;
      return data;
    }

  private:
    /*! Chunk of elements to allocate */
    class GrowingPoolElem
    {
      friend class GrowingPool;
      GrowingPoolElem(size_t elemNum) {
        this->data = PF_NEW_ARRAY(T, elemNum);
        this->next = NULL;
        this->maxElemNum = elemNum;
        this->allocated = 0;
      }
      ~GrowingPoolElem(void) {
        assert(this->data);
        PF_DELETE_ARRAY(this->data);
        if (this->next) PF_DELETE(this->next);
      }
      T *data;
      GrowingPoolElem *next;
      size_t allocated, maxElemNum;
    };
    GrowingPoolElem *current;
  };

} /* namespace pf */
#endif /* __PF_ALLOC_HPP__ */


#include "sys/alloc.hpp"
#include "sys/atomic.hpp"

/*! To track all allocations / deletions */
static pf::Atomic allocNum(0);

namespace pf
{
  void* malloc(size_t size) {
    allocNum++;
    return std::malloc(size);
  }
  void* realloc(void *ptr, size_t size) {
    if (ptr == NULL) allocNum++;
    return std::realloc(ptr, size);
  }
  void free(void *ptr) {
    if (ptr != NULL) {
      allocNum--;
      std::free(ptr);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace pf
{
  void* alignedMalloc(size_t size, size_t align) {
    void* ptr = _mm_malloc(size,align);
    FATAL_IF (!ptr && size, "memory allocation failed");
    allocNum++;
    return ptr;
  }
  void alignedFree(void *ptr) {
    if (ptr) allocNum--;
    _mm_free(ptr);
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Linux Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <malloc.h>
#include <iostream>

namespace pf
{
  void* alignedMalloc(size_t size, size_t align) {
    void* ptr = memalign(64,size);
    allocNum++;
    FATAL_IF (!ptr && size, "memory allocation failed");
    return ptr;
  }
  void alignedFree(void *ptr) {
    if (ptr) allocNum--;
    free(ptr);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// MacOS Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __MACOSX__

#include <cstdlib>

namespace pf
{
  void* alignedMalloc(size_t size, size_t align) {
    void* mem = malloc(size+(align-1)+sizeof(void*));
    FATAL_IF (!mem && size, "memory allocation failed");
    char* aligned = ((char*)mem) + sizeof(void*);
    aligned += align - ((uintptr_t)aligned & (align - 1));
    ((void**)aligned)[-1] = mem;
    return aligned;
  }
  void alignedFree(void* ptr) {
    assert(ptr);
    free(((void**)ptr)[-1]);
  }
}

#endif


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

#include "sys/platform.hpp"
#include "sys/intrinsics.hpp"

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace pf
{
  double getSeconds() {
    LARGE_INTEGER freq, val;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&val);
    return (double)val.QuadPart / (double)freq.QuadPart;
  }

  void* alignedMalloc(size_t size, size_t align) {
    void* ptr = _mm_malloc(size,align);
    FATAL_IF (!ptr && size, "memory allocation failed");
    return ptr;
  }

  void* alignedRealloc(void* ptr, size_t size, size_t align) {
    return ptr;
  }

  void alignedFree(void *ptr) {
    _mm_free(ptr);
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__UNIX__)

#include <sys/time.h>

namespace pf
{
  double getSeconds() {
    struct timeval tp; gettimeofday(&tp,NULL);
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
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
    if (!ptr && size) FATAL("memory allocation failed");
    return ptr;
  }
  void* alignedRealloc(void* ptr, size_t size, size_t align) { return realloc(ptr,size); }
  void alignedFree(void *ptr) { assert(ptr); free(ptr); }
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// MacOS Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __MACOSX__

#include <stdlib.h>

namespace pf
{
  void* alignedMalloc(size_t size, size_t align)
  {
    void* mem = malloc(size+(align-1)+sizeof(void*));
    FATAL_IF (!mem && size, "memory allocation failed");
    char* aligned = ((char*)mem) + sizeof(void*);
    aligned += align - ((uintptr_t)aligned & (align - 1));
    ((void**)aligned)[-1] = mem;
    return aligned;
  }

  void* alignedRealloc(void* ptr, size_t size, size_t align)
  {
    void* base = ((void**)ptr)[-1];
    void* newbase = realloc(base,size+(align-1)+sizeof(void*));
    FATAL_IF (newbase != base, "alignedRealloc: reducing size not supported");
    return ptr;
  }

  void alignedFree(void* ptr) {
    assert(ptr);
    free(((void**)ptr)[-1]);
  }
}

#endif



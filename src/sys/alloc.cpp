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

#include "sys/alloc.hpp"
#include "sys/atomic.hpp"
#include "sys/mutex.hpp"

#if PF_DEBUG_MEMORY
#ifdef __MSVC__
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
#endif /* PF_DEBUG_MEMORY */

#include <map>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
/// Memory debugger
////////////////////////////////////////////////////////////////////////////////
namespace pf
{

#if PF_DEBUG_MEMORY
  /*! Store each allocation data */
  struct AllocData {
    INLINE AllocData(void) {}
    INLINE AllocData(int fileName_, int functionName_, int line_, intptr_t alloc_) :
      fileName(fileName_), functionName(functionName_), line(line_), alloc(alloc_) {}
    int fileName, functionName, line;
    intptr_t alloc;
  };

  /*! Store allocation information */
  struct MemDebugger {
    MemDebugger(void) : unfreedNum(0), allocNum(0) {}
    void* insertAlloc(void *ptr, const char *file, const char *function, int line);
    void removeAlloc(void *ptr);
    void dumpAlloc(void);
    void dumpData(const AllocData &data);
    /*! Count the still unfreed allocations */
    volatile intptr_t unfreedNum;
    /*! Total number of allocations done */
    volatile intptr_t allocNum;
    /*! Sorts the file name and function name strings */
    std::tr1::unordered_map<const char*, int> staticStringMap;
    /*! Each element contains the actual string */
    std::vector<const char*> staticStringVector;
    std::map<uintptr_t, AllocData> allocMap;
    /*! Protect the memory debugger accesses */
    MutexSys mutex;
  };

  void* MemDebugger::insertAlloc(void *ptr, const char *file, const char *function, int line)
  {
    if (ptr == NULL) return ptr;
    Lock<MutexSys> lock(mutex);
    const uintptr_t iptr = (uintptr_t) ptr;
    if (UNLIKELY(allocMap.find(iptr) != allocMap.end())) {
      this->dumpData(allocMap.find(iptr)->second);
      FATAL("Pointer already in map");
    }
    const auto fileIt = staticStringMap.find(file);
    const auto functionIt = staticStringMap.find(function);
    int fileName, functionName;
    if (fileIt == staticStringMap.end()) {
      staticStringVector.push_back(file);
      staticStringMap[file] = fileName = int(staticStringVector.size()) - 1;
    } else
      fileName = staticStringMap[file];
    if (functionIt == staticStringMap.end()) {
      staticStringVector.push_back(function);
      staticStringMap[function] = functionName = int(staticStringVector.size()) - 1;
    } else
      functionName = staticStringMap[function];
    allocMap[iptr] = AllocData(fileName, functionName, line, allocNum);
    unfreedNum++;
    allocNum++;
    return ptr;
  }

  void MemDebugger::removeAlloc(void *ptr)
  {
    if (ptr == NULL) return;
    Lock<MutexSys> lock(mutex);
    const uintptr_t iptr = (uintptr_t) ptr;
    FATAL_IF(allocMap.find(iptr) == allocMap.end(), "Pointer not referenced");
    allocMap.erase(iptr);
    unfreedNum--;
  }

  void MemDebugger::dumpData(const AllocData &data) {
    std::cerr << "ALLOC " << data.alloc << ": " <<
                 "file " << staticStringVector[data.fileName] << ", " <<
                 "function " << staticStringVector[data.functionName] << ", " <<
                 "line " << data.line << std::endl;
  }

  void MemDebugger::dumpAlloc(void) {
    std::cerr << "MemDebugger: Unfreed number: " << unfreedNum << std::endl;
    for (auto it = allocMap.begin(); it != allocMap.end(); ++it)
      this->dumpData(it->second);
    std::cerr << "MemDebugger: " << staticStringVector.size()
              << " allocated static strings" << std::endl;
  }

  /*! Declare C like interface functions here */
  static MemDebugger *memDebugger = NULL;
  void* MemDebuggerInsertAlloc(void *ptr, const char *file, const char *function, int line) {
    if (memDebugger) return memDebugger->insertAlloc(ptr, file, function, line);
    return ptr;
  }
  void MemDebuggerRemoveAlloc(void *ptr) {
    if (memDebugger) memDebugger->removeAlloc(ptr);
  }
  void MemDebuggerDumpAlloc(void) {
    if (memDebugger) memDebugger->dumpAlloc();
  }
  void MemDebuggerStart(void) {
    if (memDebugger) MemDebuggerEnd();
    memDebugger = new MemDebugger;
  }
  void MemDebuggerEnd(void) {
    MemDebugger *_debug = memDebugger;
    memDebugger = NULL;
    delete _debug;
  }
#endif /* PF_DEBUG_MEMORY */
}

namespace pf
{
  void* malloc(size_t size) {
    return std::malloc(size);
  }

  void* realloc(void *ptr, size_t size) {
#if PF_DEBUG_MEMORY
    if (ptr) MemDebuggerRemoveAlloc(ptr);
#endif /* PF_DEBUG_MEMORY */
    assert(size);
    return std::realloc(ptr, size);
  }

  void free(void *ptr) {
    if (ptr != NULL) std::free(ptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __WIN32__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace pf
{
  void* alignedMalloc(size_t size, size_t align) {
    void* ptr = _mm_malloc(size,align);
    FATAL_IF (!ptr && size, "memory allocation failed");
    return ptr;
  }

  void alignedFree(void *ptr) {
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
    void* ptr = memalign(align,size);
    FATAL_IF (!ptr && size, "memory allocation failed");
    return ptr;
  }

  void alignedFree(void *ptr) {
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


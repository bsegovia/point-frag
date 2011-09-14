#include "sys/alloc.hpp"
#include "sys/atomic.hpp"
#include "sys/mutex.hpp"

#include <unordered_map>
#include <map>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
/// Memory debugger
////////////////////////////////////////////////////////////////////////////////
namespace pf
{
  /*! Count the still unfreed allocations */
  static Atomic unfreedNum(0);

#if DEBUG_MEMORY
  /*! Store each allocation data */
  struct AllocData {
    INLINE AllocData(void) {}
    INLINE AllocData(int fileName_, int functionName_, int line_, int alloc_) :
      fileName(fileName_), functionName(functionName_), line(line_), alloc(alloc_) {}
    int fileName, functionName, line, alloc;
  };

  /*! Store allocation information */
  struct MemoryDebugger {
    MemoryDebugger(void) : allocNum(0) {}
    void* insertAlloc(void *ptr, const char *file, const char *function, int line);
    void removeAlloc(void *ptr);
    void dumpAlloc(void);

    /*! Total number of allocations done */
    Atomic allocNum;
    /*! Sorts the file name and function name strings */
    std::unordered_map<const char*, int> staticStringMap;
    /*! Each element contains the actual string */
    std::vector<const char*> staticStringVector;
    std::map<uintptr_t, AllocData> allocMap;
    /*! Protect the memory debugger accesses */
    MutexSys mutex;
  };

  void* MemoryDebugger::insertAlloc(void *ptr, const char *file, const char *function, int line)
  {
    if (ptr == NULL)
      return ptr;
    Lock<MutexSys> lock(mutex);
    const uintptr_t iptr = (uintptr_t) ptr;
    FATAL_IF(allocMap.find(iptr) != allocMap.end(), "Pointer already in map");
    const auto fileIt = staticStringMap.find(file);
    const auto functionIt = staticStringMap.find(function);
    int fileName, functionName;
    if (fileIt == staticStringMap.end()) {
      staticStringVector.push_back(file);
      staticStringMap[file] = fileName = staticStringVector.size() - 1;
    } else
      fileName = staticStringMap[file];
    if (functionIt == staticStringMap.end()) {
      staticStringVector.push_back(function);
      staticStringMap[function] = functionName = staticStringVector.size() - 1;
    } else
      functionName = staticStringMap[function];
    allocMap[iptr] = AllocData(fileName, functionName, line, allocNum);
    unfreedNum++;
    allocNum++;
    return ptr;
  }

  void MemoryDebugger::removeAlloc(void *ptr)
  {
    if (ptr == NULL) return;
    Lock<MutexSys> lock(mutex);
    const uintptr_t iptr = (uintptr_t) ptr;
    FATAL_IF(allocMap.find(iptr) == allocMap.end(), "Pointer not referenced");
    allocMap.erase(iptr);
    unfreedNum--;
  }

  void MemoryDebugger::dumpAlloc(void) {
    std::cerr << "Unfreed number: " << unfreedNum << std::endl;
    for (auto it = allocMap.begin(); it != allocMap.end(); ++it) {
      const AllocData &data = it->second;
      std::cerr << "ALLOC " << data.alloc << ": " <<
                   "file " << staticStringVector[data.fileName] << ", " <<
                   "function " << staticStringVector[data.functionName] << ", " <<
                   "line " << data.line << std::endl;
    }
    std::cerr << staticStringVector.size() << " allocated static strings" << std::endl;
  }

  /*! Declare C like interface functions here */
  static MemoryDebugger *memoryDebugger = NULL;
  void* insertAlloc(void *ptr, const char *file, const char *function, int line) {
    if (memoryDebugger) return memoryDebugger->insertAlloc(ptr, file, function, line);
    return ptr;
  }
  void removeAlloc(void *ptr) {
    if (memoryDebugger) memoryDebugger->removeAlloc(ptr);
  }
  void dumpAlloc(void) {
    if (memoryDebugger) memoryDebugger->dumpAlloc();
  }
  void startMemoryDebugger(void) {
    if (memoryDebugger)
      endMemoryDebugger();
    memoryDebugger = new MemoryDebugger;
  }
  void endMemoryDebugger(void) {
    MemoryDebugger *_debug = memoryDebugger;
    memoryDebugger = NULL;
    delete _debug;
  }
#endif /* DEBUG_MEMORY */
}

namespace pf
{
  void* malloc(size_t size) {
    unfreedNum++;
    return std::malloc(size);
  }

  void* realloc(void *ptr, size_t size) {
    if (ptr == NULL) unfreedNum++;
    return std::realloc(ptr, size);
  }

  void free(void *ptr) {
    if (ptr != NULL) {
      unfreedNum--;
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
    unfreedNum++;
    return ptr;
  }

  void alignedFree(void *ptr) {
    if (ptr) unfreedNum--;
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
    unfreedNum++;
    FATAL_IF (!ptr && size, "memory allocation failed");
    return ptr;
  }

  void alignedFree(void *ptr) {
    if (ptr) unfreedNum--;
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


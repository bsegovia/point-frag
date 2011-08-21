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

#ifndef __PF_BUILDER_H__
#define __PF_BUILDER_H__

namespace pf
{
  /*! Builder base class. This class implements an efficient
   *  multi-threaded memory allocation scheme used by the builders. A
   *  builder thread can request new memory for a number of nodes or
   *  triangles (threadAllocXXX function). The per thread allocator
   *  allocates from its current memory block or requests a new block
   *  from the slower global allocator (globalAllocXXX function) when
   *  this block is full. */
  class Builder
  {
  public:

    /*! Builder default construction. */
    Builder () : atomicNextNode(0), atomicNextPrimitive(0) {}

    /*! Builders need a virtual destructor. */
    virtual ~Builder() {}

    /*! Allocation Block size. Number of nodes and triangles to
     *  request from the global allocator when the thread memory block
     *  is empty. */
    enum { allocBlockSize = 4096 };

    /*! Per thread structure holding the current memory block. */
    struct ThreadAllocator {
      ThreadAllocator () : cur(0), end(0) {}
    public:
      size_t cur;                        //!< Current location of the allocator.
      size_t end;                        //!< End of the memory block.
      char align[64-3*sizeof(size_t)];   //!< Aligns structure to cache line size.
    };

  public:

    size_t allocatedNodes;                 //!< Total number of nodes available for allocation
    Atomic atomicNextNode;                 //!< Next available node for global allocator.
    ThreadAllocator threadNextNode[128];   //!< thread local allocator for nodes

    /*! Global atomic node allocator. */
    INLINE size_t globalAllocNodes(size_t num = 1) {
      size_t end = atomicNextNode += num;
    if (end > allocatedNodes) throw std::runtime_error("Builder: out of node memory");
      return end-num;
    }

    /*! Thread local allocation of nodes. */
    INLINE size_t threadAllocNodes(size_t tid, size_t num = 1)
    {
      ThreadAllocator& alloc = threadNextNode[tid];
      alloc.cur += num;
      if (alloc.cur < alloc.end) return alloc.cur - num;
      alloc.cur = globalAllocNodes(allocBlockSize);
      alloc.end = alloc.cur+allocBlockSize;
      alloc.cur += num;
      if (alloc.cur < alloc.end) return alloc.cur - num;
      throw std::runtime_error("Builder: too large node block requested");
      return 0;
    }

  public:

    size_t allocatedPrimitives;               //!< Total number of primitives available for allocation.
    Atomic atomicNextPrimitive;               //!< Next available primitive for global allocator.
    ThreadAllocator threadNextPrimitive[128]; //!< Thread local allocator for primitives.

    /*! Global atomic primitive allocator. */
    INLINE size_t globalAllocPrimitives(size_t num = 1) {
      size_t end = atomicNextPrimitive += num;
      if (end > allocatedPrimitives) throw std::runtime_error("Builder: out of primitive memory");
      return end-num;
    }

    /*! Thread local allocation of primitives. */
    INLINE size_t threadAllocPrimitives(size_t tid, size_t num = 1)
    {
      ThreadAllocator& alloc = threadNextPrimitive[tid];
      alloc.cur += num;
      if (alloc.cur < alloc.end) return alloc.cur - num;
      alloc.cur = globalAllocPrimitives(allocBlockSize);
      alloc.end = alloc.cur+allocBlockSize;
      alloc.cur += num;
      if (alloc.cur < alloc.end) return alloc.cur - num;
      throw std::runtime_error("Builder: too large primitive block requested");
      return 0;
    }
  };
}

#endif

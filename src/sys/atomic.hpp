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

#ifndef __PF_ATOMIC_HPP__
#define __PF_ATOMIC_HPP__

#include "sys/intrinsics.hpp"

namespace pf
{
  template <typename T>
  struct AtomicInternal {
  protected:
    AtomicInternal(const AtomicInternal&); // don't implement
    AtomicInternal& operator= (const AtomicInternal&); // don't implement

  public:
    INLINE AtomicInternal(void) {}
    INLINE AtomicInternal(T data) : data(data) {}
    INLINE AtomicInternal& operator =(const T input) { data = input; return *this; }
    INLINE operator T() const { return data; }

  public:
    INLINE friend T operator+= (AtomicInternal& value, T input) { return atomic_add(&value.data, input) + input; }
    INLINE friend T operator++ (AtomicInternal& value) { return atomic_add(&value.data,  1) + 1; }
    INLINE friend T operator-- (AtomicInternal& value) { return atomic_add(&value.data, -1) - 1; }
    INLINE friend T operator++ (AtomicInternal& value, int) { return atomic_add(&value.data,  1); }
    INLINE friend T operator-- (AtomicInternal& value, int) { return atomic_add(&value.data, -1); }
    INLINE friend T cmpxchg    (AtomicInternal& value, const T v, const T c) { return atomic_cmpxchg(&value.data,v,c); }

  private:
    volatile T data;
  };

  typedef AtomicInternal<atomic32_t> Atomic32;
  typedef AtomicInternal<atomic_t> Atomic;
}

#endif /* __PF_ATOMIC_HPP__ */


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
  struct Atomic {
  protected:
    Atomic(const Atomic&); // don't implement
    Atomic& operator= (const Atomic&); // don't implement

  public:
    INLINE Atomic(void) {}
    INLINE Atomic(atomic_t data) : data(data) {}
    INLINE Atomic& operator =(const atomic_t input) { data = input; return *this; }
    INLINE operator atomic_t() const { return data; }

  public:
    INLINE friend atomic_t operator+= (Atomic& value, atomic_t input) { return atomic_add(&value.data, input) + input; }
    INLINE friend atomic_t operator++ (Atomic& value) { return atomic_add(&value.data,  1) + 1; }
    INLINE friend atomic_t operator-- (Atomic& value) { return atomic_add(&value.data, -1) - 1; }
    INLINE friend atomic_t operator++ (Atomic& value, int) { return atomic_add(&value.data,  1); }
    INLINE friend atomic_t operator-- (Atomic& value, int) { return atomic_add(&value.data, -1); }
    INLINE friend atomic_t cmpxchg    (Atomic& value, const atomic_t v, const atomic_t c) { return atomic_cmpxchg(&value.data,v,c); }

  private:
    volatile atomic_t data;
  };
}

#endif /* __PF_ATOMIC_HPP__ */


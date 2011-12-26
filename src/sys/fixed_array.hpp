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

#ifndef __PF_FIXED_ARRAY_HPP__
#define __PF_FIXED_ARRAY_HPP__

#include "platform.hpp"
#include <cstring>

namespace pf
{
  /*! Regular C array but with bound checks */
  template<typename T, size_t N>
  class fixed_array
  {
  public:
    /*! Do not initialize the data */
    fixed_array(void) {}
    /*! Copy the input array */
    fixed_array(const T array[N]) { std::memcpy(elem, array, N * sizeof(T)); }
    /*! First element (non const) */
    T* begin(void) { return &elem[0]; }
    /*! First non-valid element (non const) */
    T* end(void) { return begin() + N; }
    /*! First element (const) */
    const T* begin(void) const { return &elem[0]; }
    /*! First non-valid element (const) */
    const T* end(void) const { return begin() + N; }
    /*! Number of elements in the array */
    size_t size(void) const { return N; }
    /*! Get the pointer to the data (non-const) */
    T* data(void) { return &elem[0]; }
    /*! Get the pointer to the data (const) */
    const T* data(void) const { return &elem[0]; }
    /*! First element (const) */
    const T& front(void) const { return *begin(); }
    /*! Last element (const) */
    const T& back(void) const { return *(end() - 1); }
    /*! First element (non-const) */
    T& front(void) { return *begin(); }
    /*! Last element (non-const) */
    T& back(void) { return *(end() - 1); }
    /*! Get element at position index (with bound check) */
    INLINE T& operator[] (size_t index) {
      PF_ASSERT(index < size());
      return elem[index];
    }
    /*! Get element at position index (with bound check) */
    INLINE const T& operator[] (size_t index) const {
      PF_ASSERT(index < size());
      return elem[index];
    }
  private:
    T elem[N];            //!< Store the elements
    STATIC_ASSERT(N > 0); //!< zero element is not allowed
    PF_CLASS(fixed_array);
  };

} /* namespace pf */

#endif /* __PF_FIXED_ARRAY_HPP__ */



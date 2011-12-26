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

#ifndef __PF_VECTOR_HPP__
#define __PF_VECTOR_HPP__

#include "sys/platform.hpp"
#include <vector>

namespace pf
{
  /*! Add bound checks to the standard vector class and use the internal
   *  allocator
   */
  template<class T>
  class vector : public std::vector<T, Allocator<T>>
  {
  public:
    // Typedefs
    typedef std::vector<T, Allocator<T>>       parent_type;
    typedef Allocator<T>                       allocator_type;
    typedef typename allocator_type::size_type size_type;

    /*! Default constructor */
    INLINE explicit vector(const allocator_type &a = allocator_type()) :
      parent_type(a) {}
    /*! Copy constructor */
    INLINE vector(const vector &x) : parent_type(x) {}
    /*! Repetitive sequence constructor */
    INLINE explicit vector(size_type n,
                           const T& value= T(),
                           const allocator_type &a = allocator_type()) :
      parent_type(n, value, a) {}
    /*! Iteration constructor */
    template <class InputIterator>
    INLINE vector(InputIterator first,
                  InputIterator last,
                  const allocator_type &a = allocator_type()) :
      parent_type(first, last, a) {}
    /*! Get element at position index (with a bound check) */
    T &operator[] (size_t index) {
      PF_ASSERT(index < this->size());
      return parent_type::operator[] (index);
    }
    /*! Get element at position index (with a bound check) */
    const T &operator[] (size_t index) const {
      PF_ASSERT(index < this->size());
      return parent_type::operator[] (index);
    }
    PF_CLASS(vector);
  };
} /* namespace pf */

#endif /* __PF_VECTOR_HPP__ */


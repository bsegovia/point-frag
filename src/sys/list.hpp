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

#ifndef __PF_LIST_HPP__
#define __PF_LIST_HPP__

#include "sys/platform.hpp"
#include <list>

namespace pf
{
  /*! Use custom allocator instead of std one */
  template <typename T>
  class list : public std::list<T, Allocator<T>>
  {
  public:
    // Typedefs
    typedef T value_type;
    typedef Allocator<value_type> allocator_type;
    typedef std::list<T, allocator_type> parent_type;
    typedef typename allocator_type::size_type size_type;

    /*! Default constructor */
    INLINE explicit list(const allocator_type &a = allocator_type()) :
      parent_type(a) {}
    /*! Repetitive constructor */
    INLINE explicit list(size_type n,
                         const T &value = T(),
                         const allocator_type &a = allocator_type()) :
      parent_type(n, value, a) {}
    /*! Iteration constructor */
    template <class InputIterator>
    INLINE list(InputIterator first,
                InputIterator last,
                const allocator_type &a = allocator_type()) :
      parent_type(first, last, a) {}
    /*! Copy constructor */
    INLINE list(const list &x) : parent_type(x) {}
    PF_CLASS(list);
  };
} /* namespace pf */

#endif /* __PF_LIST_HPP__ */


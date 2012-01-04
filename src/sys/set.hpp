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

#ifndef __PF_SET_HPP__
#define __PF_SET_HPP__

#include "sys/platform.hpp"
#include <set>

namespace pf
{
  /*! Add our custom allocator to std::set */
  template<class Key, class Pred = std::less<Key>>
  class set : public std::set<Key,Pred,Allocator<Key>>
  {
  public:
    // Typedefs
    typedef Key value_type;
    typedef Allocator<value_type> allocator_type;
    typedef std::set<Key,Pred,Allocator<Key>> parent_type;
    typedef Key key_type;
    typedef Pred key_compare;

    /*! Default constructor */
    INLINE set(const key_compare &comp = key_compare(),
               const allocator_type &a = allocator_type()) :
      parent_type(comp, a) {}
    /*! Iteration constructor */
    template<class InputIterator>
    INLINE set(InputIterator first,
               InputIterator last,
               const key_compare &comp = key_compare(),
               const allocator_type& a = allocator_type()) :
      parent_type(first, last, comp, a) {}
    /*! Copy constructor */
    INLINE set(const set& x) : parent_type(x) {}
    PF_CLASS(set);
  };

} /* namespace pf */

#endif /* __PF_SET_HPP__ */


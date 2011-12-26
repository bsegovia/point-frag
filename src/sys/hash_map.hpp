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

#ifndef __PF_HASH_MAP_HPP__
#define __PF_HASH_MAP_HPP__

#include "sys/platform.hpp"

#ifdef __MSVC__
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif /* __MSVC__ */

namespace pf
{
  /*! Add specific allocator to the hash map */
  template <class Key,
            class T,
            class Hash = std::hash<Key>,
            class Pred = std::equal_to<Key>>
  class hash_map : public std::tr1::unordered_map<Key,T,Hash,Pred,Allocator<std::pair<const Key,T>>>
  {
  public:
    // Typedefs
    typedef std::pair<const Key, T> value_type;
    typedef Allocator<value_type> allocator_type;
    typedef std::tr1::unordered_map<Key,T,Hash,Pred,allocator_type> parent_type;
    typedef typename allocator_type::size_type size_type;
    typedef Key key_type;
    typedef T mapped_type;
    typedef Hash hasher;
    typedef Pred key_equal;

    /*! Default constructor */
    INLINE explicit hash_map(size_type n = 3,
                             const hasher& hf = hasher(),
                             const key_equal& eql = key_equal(),
                             const allocator_type& a = allocator_type()) :
      parent_type(n, hf, eql, a) {}
    /*! Iteration constructor */
    template <class InputIterator>
    INLINE hash_map(InputIterator first,
                    InputIterator last,
                    size_type n = 3,
                    const hasher& hf = hasher(),
                    const key_equal& eql = key_equal(),
                    const allocator_type& a = allocator_type()) :
      parent_type(first,last,n,hf,eql,a) {}
    /*! Copy constructor */
    INLINE hash_map(const hash_map &other) : parent_type(other) {}
  };
} /* namespace pf */

#endif /* __PF_HASH_MAP_HPP__ */


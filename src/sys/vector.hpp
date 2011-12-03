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
  /*! Add bound checks */
  template<class T>
  class vector : public std::vector<T>
  {
  public:
    /*! Get element at position index (with a bound check) */
    T &operator[] (size_t index) {
      PF_ASSERT(index < this->size());
      return std::vector<T>::operator[] (index);
    }
    /*! Get element at position index (with a bound check) */
    const T &operator[] (size_t index) const {
      PF_ASSERT(index < this->size());
      return std::vector<T>::operator[] (index);
    }
  };
} /* namespace pf */

#endif /* __PF_VECTOR_HPP__ */


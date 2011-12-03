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

#ifndef __PF_ARRAY_HPP__
#define __PF_ARRAY_HPP__

#include "sys/platform.hpp"
#include <vector>

namespace pf
{
  /*! Non resizable array with no checking. We make it non-copiable right now
   *  since we do not want to implement an expensive deep copy
   */
  template<class T>
  class array : NonCopyable
  {
  public:
    /*! Create an empty array */
    INLINE array(void) : elem(NULL), elemNum(0) {}
    /*! Allocate an array with elemNum allocated elements */
    INLINE array(size_t elemNum) : elem(NULL), elemNum(0) { this->init(elemNum); }
    /*! Delete the allocated elements */
    INLINE ~array(void) { PF_SAFE_DELETE_ARRAY(elem); }
    /*! Free the already allocated elements and allocate a new array */
    INLINE void init(size_t elemNum_) {
      PF_SAFE_DELETE_ARRAY(elem);
      this->elem = PF_NEW_ARRAY(T, elemNum_);
      this->elemNum = elemNum_;
    }
    /*! Steal the pointer. The array becomes emtpy */
    INLINE T *steal(void) {
      T *stolen = this->elem;
      this->elem = NULL;
      this->elemNum = 0;
      return stolen;
    }
    /*! First element */
    INLINE T *begin(void) { return this->elem; }
    /*! First non-valid element */
    INLINE T *end(void) { return this->elem + elemNum; }
    /*! Get element at position index (with a bound check) */
    INLINE T &operator[] (size_t index) {
      PF_ASSERT(index < elemNum);
      return elem[index];
    }
    /*! Get element at position index (with bound check) */
    INLINE const T &operator[] (size_t index) const {
      PF_ASSERT(index < elemNum);
      return elem[index];
    }
    /*! Return the number of elements */
    INLINE size_t size(void) const { return this->elemNum; }
  private:
    T *elem;        //!< Points to the elements
    size_t elemNum; //!< Number of elements in the array
  };
} /* namespace pf */

#endif /* __PF_ARRAY_HPP__ */


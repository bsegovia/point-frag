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

#ifndef __SAMPLE_HPP__
#define __SAMPLE_HPP__

#include "math/vec.hpp"
#include <iostream>

namespace pf {

  /*! Sample representation. A sample has a location and a probability
   *  density it got sampled with. */
  template<typename T> struct Sample
  {
    inline Sample           () {}
    inline Sample           ( const Sample& other ) { value = other.value; pdf = other.pdf; }
    inline Sample& operator=( const Sample& other ) { value = other.value; pdf = other.pdf; return *this; }

    inline Sample (const T& value, float pdf = 1.f) : value(value), pdf(pdf) {}

    inline operator const T&( ) const { return value; }
    inline operator       T&( )       { return value; }

  public:
    T value;    //!< location of the sample
    float pdf;  //!< probability density of the sample
  };

  /*! output operator */
  template<typename T> inline std::ostream& operator<<(std::ostream& cout, const Sample<T>& s) {
    return cout << "{ value = " << s.value << ", pdf = " << s.pdf << "}";
  }

  /*! shortcuts for common sample types */
  typedef Sample<int>   Sample1i;
  typedef Sample<float> Sample1f;
  typedef Sample<vec2f> Sample2f;
  typedef Sample<vec3f> Sample3f;
}

#endif /* __SAMPLE_HPP__ */


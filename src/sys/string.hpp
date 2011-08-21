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

#ifndef __PF_STRING_H__
#define __PF_STRING_H__

#include "sys/platform.hpp"
#include "sys/filename.hpp"

#include <cstring>
#include <string>
#include <sstream>
#include <fstream>

namespace std
{
  string strlwr(const string& s);
  string strupr(const string& s);

  template<typename T> INLINE string stringOf( const T& v) {
    stringstream s; s << v; return s.str();
  }
}
namespace pf
{
  /*! Load a file from its path and copies it into a string */
  std::string loadFile(const FileName &path);
  /*! Load a file from a stream and copies it into a string */
  std::string loadFile(std::ifstream &stream);
}
#endif

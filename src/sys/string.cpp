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

#include "sys/string.hpp"
#include "sys/filename.hpp"

#include <cstdio>
#include <cctype>
#include <algorithm>

namespace std
{
  char to_lower(char c) { return char(tolower(int(c))); }
  char to_upper(char c) { return char(toupper(int(c))); }
  string strlwr(const string& s) {
    string dst(s);
    std::transform(dst.begin(), dst.end(), dst.begin(), to_lower);
    return dst;
  }
  string strupr(const string& s) {
    string dst(s);
    std::transform(dst.begin(), dst.end(), dst.begin(), to_upper);
    return dst;
  }
}
namespace pf
{
  std::string loadFile(const FileName &path)
  {
    std::ifstream stream(path, std::ios::in);
    if (stream.is_open() == false)
      return std::string();
    std::string str = loadFile(stream);
    stream.close();
    return str;
  }

  std::string loadFile(std::ifstream &stream)
  {
    assert(stream.is_open());
    std::string line;
    std::stringstream text;
    while (std::getline(stream, line))
      text << "\n" << line;
    stream.close();
    return text.str();
  }
}


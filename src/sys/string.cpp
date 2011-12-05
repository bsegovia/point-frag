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
#include <istream>
#include <fstream>
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
  /* $Id: strtok_r.c,v 1.1 2003/12/03 15:22:23 chris_reid Exp $ */
  /*
   * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska Hgskolan
   * (Royal Institute of Technology, Stockholm, Sweden).
   * All rights reserved.
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions
   * are met:
   *
   * 1. Redistributions of source code must retain the above copyright
   *    notice, this list of conditions and the following disclaimer.
   *
   * 2. Redistributions in binary form must reproduce the above copyright
   *    notice, this list of conditions and the following disclaimer in the
   *    documentation and/or other materials provided with the distribution.
   *
   * 3. Neither the name of the Institute nor the names of its contributors
   *    may be used to endorse or promote products derived from this software
   *    without specific prior written permission.
   *
   * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
   * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
   * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   * SUCH DAMAGE.
   */
  char* tokenize(char *s1, const char *s2, char **lasts)
  {
   char *ret;

   if (s1 == NULL)
     s1 = *lasts;
   while(*s1 && strchr(s2, *s1))
     ++s1;
   if(*s1 == '\0')
     return NULL;
   ret = s1;
   while(*s1 && !strchr(s2, *s1))
     ++s1;
   if(*s1)
     *s1++ = '\0';
   *lasts = s1;
   return ret;
  }

  bool strequal(const char *s1, const char *s2) {
    if (strcmp(s1, s2) == 0) return true;
    return false;
  }

  bool contains(const char *haystack, const char *needle) {
    if (strstr(haystack, needle) == NULL) return false;
    return true;
  }

  std::string loadFile(const FileName &path)
  {
    std::ifstream stream(path, std::istream::in);
    if (stream.is_open() == false)
      return std::string();
    std::string str = loadFile(stream);
    stream.close();
    return str;
  }

  std::string loadFile(std::ifstream &stream)
  {
    PF_ASSERT(stream.is_open());
    std::string line;
    std::stringstream text;
    while (std::getline(stream, line))
      text << "\n" << line;
    stream.close();
    return text.str();
  }
}


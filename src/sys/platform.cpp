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

#include "sys/platform.hpp"
#include "sys/intrinsics.hpp"
#include <string>

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __WIN32__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
extern "C" char* strtok_r(char *s1, const char *s2, char **lasts)
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

namespace pf
{
  double getSeconds() {
    LARGE_INTEGER freq, val;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&val);
    return (double)val.QuadPart / (double)freq.QuadPart;
  }

  void FATAL(const std::string &msg) {
    std::cerr << msg << std::endl;
    MessageBox(NULL, msg.c_str(), "Fatal Error", MB_OK | MB_ICONEXCLAMATION);
    PF_ASSERT(0);
#ifdef __GNUC__
    exit(-1);
#else
    _exit(-1);
#endif /* __GNUC__ */
  }

} /* namespace pf */
#endif /* __WIN32__ */

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__UNIX__)

#include <sys/time.h>

namespace pf
{
  double getSeconds() {
    struct timeval tp; gettimeofday(&tp,NULL);
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
  }

  void FATAL(const std::string &msg) {
    std::cerr << msg << std::endl;
    PF_ASSERT(0);
    _exit(-1);
  }
} /* namespace pf */

#endif /* __UNIX__ */


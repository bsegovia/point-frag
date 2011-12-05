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


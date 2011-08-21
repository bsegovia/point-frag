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

#include "sys/sysinfo.hpp"

////////////////////////////////////////////////////////////////////////////////
/// All Platforms
////////////////////////////////////////////////////////////////////////////////

namespace pf
{
  /* return platform name */
  std::string getPlatformName() {
#if defined(__LINUX__) && !defined(__X86_64__)
    return "Linux (32bit)";
#elif defined(__LINUX__) && defined(__X86_64__)
    return "Linux (64bit)";
#elif defined(__FREEBSD__) && !defined(__X86_64__)
    return "FreeBSD (32bit)";
#elif defined(__FREEBSD__) && defined(__X86_64__)
    return "FreeBSD (64bit)";
#elif defined(__CYGWIN__) && !defined(__X86_64__)
    return "Cygwin (32bit)";
#elif defined(__CYGWIN__) && defined(__X86_64__)
    return "Cygwin (64bit)";
#elif defined(__WIN32__) && !defined(__X86_64__)
    return "Windows (32bit)";
#elif defined(__WIN32__) && defined(__X86_64__)
    return "Windows (64bit)";
#elif defined(__MACOSX__) && !defined(__X86_64__)
    return "MacOS (32bit)";
#elif defined(__MACOSX__) && defined(__X86_64__)
    return "MacOS (64bit)";
#elif defined(__UNIX__) && !defined(__X86_64__)
    return "Unix (32bit)";
#elif defined(__UNIX__) && defined(__X86_64__)
    return "Unix (64bit)";
#else
    return "Unknown";
#endif
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __WIN32__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace pf
{
  /* get the full path to the running executable */
  std::string getExecutableFileName() {
    char filename[1024];
    if (!GetModuleFileName(NULL, filename, sizeof(filename))) return std::string();
    return std::string(filename);
  }

  /* return the number of logical threads of the system */
  int getNumberOfLogicalThreads() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Linux Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__

#include <stdio.h>
#include <unistd.h>

namespace pf
{
  /* get the full path to the running executable */
  std::string getExecutableFileName() {
    char pid[32]; sprintf(pid, "/proc/%d/exe", getpid());
    char buf[1024];
    int bytes = readlink(pid, buf, sizeof(buf)-1);
    if (bytes != -1) buf[bytes] = '\0';
    return std::string(buf);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// MacOS Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __MACOSX__

#include <mach-o/dyld.h>

namespace pf
{
  /* get the full path to the running executable */
  std::string getExecutableFileName()
  {
    char buf[1024];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return std::string();
    return std::string(buf);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__UNIX__)

#include <unistd.h>

namespace pf
{
  /* return the number of logical threads of the system */
  int getNumberOfLogicalThreads() {
    return sysconf(_SC_NPROCESSORS_CONF);
  }
}
#endif


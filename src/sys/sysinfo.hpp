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

#ifndef __PF_SYSINFO_H__
#define __PF_SYSINFO_H__

#include "sys/platform.hpp"

#include <string>

namespace pf
{
  /*! get the full path to the running executable */
  std::string getExecutableFileName();

  /*! return platform name */
  std::string getPlatformName();

  /*! return the number of logical threads of the system */
  int getNumberOfLogicalThreads();
}

#endif

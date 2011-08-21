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

#include "sys/library.hpp"
#include "sys/sysinfo.hpp"
#include "sys/filename.hpp"

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__WIN32__)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace pf
{
  /* opens a shared library */
  lib_t openLibrary(const std::string& file)
  {
    std::string fullName = file+".dll";
    HMODULE handle = LoadLibraryA(fullName.c_str());
    if (handle) return lib_t(handle);
    handle = LoadLibrary((getExecutableFileName() + fullName).c_str());
    return lib_t(handle);
  }

  /* returns address of a symbol from the library */
  void* getSymbol(lib_t lib, const std::string& sym) {
    return GetProcAddress(HMODULE(lib),sym.c_str());
  }

  /* closes the shared library */
  void closeLibrary(lib_t lib) {
    FreeLibrary(HMODULE(lib));
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__UNIX__)

#include <dlfcn.h>

namespace pf
{
  /* opens a shared library */
  lib_t openLibrary(const std::string& file)
  {
#if defined(__MACOSX__)
    std::string fullName = "lib"+file+".dylib";
#else
    std::string fullName = "lib"+file+".so";
#endif
    void* lib = dlopen(fullName.c_str(),RTLD_NOW);
    if (lib) return lib_t(lib);
    lib = dlopen((getExecutableFileName() + fullName).c_str(),RTLD_NOW);
    return lib_t(lib);
  }

  /* returns address of a symbol from the library */
  void* getSymbol(lib_t lib, const std::string& sym) {
    return dlsym(lib,sym.c_str());
  }

  /* closes the shared library */
  void closeLibrary(lib_t lib) {
    dlclose(lib);
  }
}
#endif

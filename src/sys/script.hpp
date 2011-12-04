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

#ifndef __PF_SCRIPT_HPP__
#define __PF_SCRIPT_HPP__

#include "platform.hpp"
#include <string>

namespace pf
{
  /*! Provide information about how the script run */
  struct ScriptStatus
  {
    std::string msg; //!< Error message if the script failed
    bool success;    //!< Indicate if the script properly run
  };

  /*! Bare metal script system interface. We will use LuaJIT for that. We have
   *  two ways to run code:
   *  - sandboxed: a safe private environment "pf" we created is used as
   *    global environment
   *  - non protected: the code justs run in the *real* global environment
   */
  class ScriptSystem
  {
  public:
    virtual ~ScriptSystem(void);
    /*! Execute the given string */
    virtual void run(const char *str, ScriptStatus &status) = 0;
    /*! Open the file and run it */
    virtual void runFile(const char *path, ScriptStatus &status) = 0;
    /*! Execute the given string in the *non* sand boxed environment */
    virtual void runNonProtected(const char *str, ScriptStatus &status) = 0;
    /*! Open the file and run it */
    virtual void runFileNonProtected(const char *path, ScriptStatus &status) = 0;
  };

  /*! Instantiate a Lua state basically */
  ScriptSystem *LuaScriptSystemCreate(void);

} /* namespace pf */

#endif /* __PF_SCRIPT_HPP__ */


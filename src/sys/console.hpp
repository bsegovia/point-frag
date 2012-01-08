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

#ifndef __PF_CONSOLE_HPP__
#define __PF_CONSOLE_HPP__

#include "sys/platform.hpp"

namespace pf
{
  class InputControl; // Handle input controls (keyboard/mouse...)
  class ScriptSystem; // Run the code provided by the user

  /*! Interactive command prompt with history and auto-completion */
  class Console : public NonCopyable
  {
  public:
    /*! The script system is responsible for running the provided commands */
    Console(ScriptSystem &scriptSystem);
    /*! Destructor */
    virtual ~Console(void);
    /*! Update the internal state and possibly running code with the scripting
     *  system
     */
    virtual void update(const InputControl &input) = 0;
    /*! Change the history size */
    virtual void setHistorySize(uint32 size) = 0;
    /*! Add a new completion candidate */
    virtual void addCompletion(const std::string &str) = 0;
  protected:
    ScriptSystem &scriptSystem;
  };

  /*! Create a new console */
  Console *ConsoleNew(ScriptSystem &scriptSystem);
  /*! Delete a console */
  void ConsoleDelete(Console *);

} /* namespace pf */

#endif /* __PF_CONSOLE_HPP__ */

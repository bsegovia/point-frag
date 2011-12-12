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

#include "sys/script.hpp"
#include "sys/command.hpp"
#include "sys/logging.hpp"
#include "sys/tasking.hpp"
#include "utest/utest.hpp"

using namespace pf;

VARI(coucou, 0, 2, 3, "coucou");

void utest_lua(void)
{
  ScriptSystem *scriptSystem = LuaScriptSystemCreate();
  ScriptStatus status;
  scriptSystem->run("local x = 0", status);
  ConsoleSystemStart(*scriptSystem);

  // This one is a copy of the cvars that we can safely use in the rest of the
  // code
  Ref<ConVarSystem> cvar = ConVarSystem::global->clone();

  // Run some code. This may modify console variables
  scriptSystem->run("cv.coucou = 1", status);
  if (!status.success) PF_ERROR(status.msg);
  scriptSystem->runNonProtected("print(pf.cv.coucou)", status);
  if (!status.success) PF_ERROR(status.msg);

  // If a variable has been modified, copy the console variables into a new
  // system
  if (ConVarSystem::global->isModified())
    cvar = ConVarSystem::global->clone();
  if (coucou(cvar) == 1) PF_MSG("coucou is equal to 1");

  ConsoleSystemEnd();
  PF_DELETE(scriptSystem);
}

UTEST_REGISTER(utest_lua)


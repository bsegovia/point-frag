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

using namespace pf;
//VARI(hop, 1, 2, 3, NULL);
//PF_SCRIPT int hop0 = 0;
  VARI(c0oucou, 0, 2, 3, "coucou");

int main(int argc, char *argv[])
{
  ScriptSystem *scriptSystem = LuaScriptSystemCreate();
  ScriptStatus status;
  scriptSystem->run("local x = 0", status);
  PF_DELETE(scriptSystem);
}


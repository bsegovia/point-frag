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

#include "utest/utest.hpp"
#include "sys/console.hpp"
#include "sys/logging.hpp"
#include "sys/windowing.hpp"
#include "sys/script.hpp"
#include <string>

namespace pf
{
  class UTestConsoleDisplay : public ConsoleDisplay
  {
    virtual void line(const std::string &line) { std::cout << '\r' << line; }
    virtual void out(const std::string &str) { std::cout << str << std::endl; }
  };

} /* namespace pf */

void utest_console(void)
{
  using namespace pf;
  WinOpen(640, 480);
  ScriptSystem *scriptSystem = LuaScriptSystemCreate();
  UTestConsoleDisplay *display = PF_NEW(UTestConsoleDisplay);
  Console *console = ConsoleNew(*scriptSystem, *display);

  PF_DELETE(console);
  PF_DELETE(scriptSystem);
  PF_DELETE(display);
  WinClose();
}

UTEST_REGISTER(utest_console);


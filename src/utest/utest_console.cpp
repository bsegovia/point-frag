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
  /*! Output everything in the terminal */
  class UTestConsoleDisplay : public ConsoleDisplay
  {
  public:
    UTestConsoleDisplay (void) {
      last = lastBlink = getSeconds();
      cursor = 0;
    }
    virtual void line(Console &console, const std::string &line) {
      const double curr = getSeconds();
      std::string patched = line;
      if (curr - lastBlink > .5) {
        cursor ^= 1;
        lastBlink = curr;
      }
      const uint32 pos = console.cursorPosition();
      if (cursor) {
        if (pos >= patched.size())
          patched.push_back('_');
        else
          patched[pos] = '_';
      }
      if (curr - last > 0.02) {
        std::cout << '\r' << "> " << patched;
        for (int i = 0; i < 80; ++i) std::cout << ' ';
        std::cout << '\r';
        fflush(stdout);
        last = curr;
      }
    }
    virtual void out(Console &console, const std::string &str) {
      std::cout << str << std::endl;
    }
    double last;
    double lastBlink;
    uint32 cursor;
  };
} /* namespace pf */

void utest_console(void)
{
  using namespace pf;
  WinOpen(640, 480);
  ScriptSystem *scriptSystem = LuaScriptSystemCreate();
  UTestConsoleDisplay *display = PF_NEW(UTestConsoleDisplay);
  Console *console = ConsoleNew(*scriptSystem, *display);
  console->addCompletion("while");
  console->addCompletion("whilewhile");
  for (;;) {
    Ref<InputControl> input = PF_NEW(InputControl);
    input->processEvents();
    if (input->getKey(PF_KEY_ASCII_ESC))
      break;
    console->update(*input);
    WinSwapBuffers();
  }
  PF_DELETE(console);
  PF_DELETE(scriptSystem);
  PF_DELETE(display);
  WinClose();
}

UTEST_REGISTER(utest_console);


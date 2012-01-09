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

#include "game.hpp"
#include "camera.hpp"
#include "game_event.hpp"
#include "game_frame.hpp"
#include "renderer/renderer_context.hpp"
#include "sys/alloc.hpp"
#include "sys/tasking.hpp"
#include "sys/windowing.hpp"
#include "sys/logging.hpp"

#include <cstdio>
#include <iostream>

namespace pf
{
  static const int defaultWidth = 800, defaultHeight = 600;
  static const char *objName = "arabic_city_II.obj";
  //static const char *objName = "f000.obj";
  //static const char *objName = "small.obj";
  //static const char *objName = "Arabic_City.obj";
  //static const char *objName = "conference.obj";
  //static const char *objName = "sibenik.obj";
  //static const char *objName = "sponza.obj";
  RnContext renderer = NULL;
  RnObj renderObj = NULL;

  static void GameStart(int argc, char **argv) {
    WinOpen(defaultWidth, defaultHeight);
    renderer = rnContextNew();
    renderObj = rnObjNew(renderer, objName);
    rnObjProperties(renderObj, RN_OBJ_OCCLUDER);
    rnObjCompile(renderObj);
  }

  static void GameEnd(void) {
    rnObjDelete(renderObj);
    rnContextDelete(renderer);
    WinClose();
  }
} /* namespace pf */

void game(int argc, char **argv)
{
  using namespace pf;
  GameStart(argc, argv);

  // We create a dummy frame such that a previous frame always exists in the
  // system. This makes everything a lot easier to handle. We do not destroy it
  // since it is referenced counted
  GameFrame *dummyFrame = PF_NEW(GameFrame, defaultWidth, defaultHeight);
  Task *frameTask = PF_NEW(TaskGameFrame, *dummyFrame);
  frameTask->scheduled();
  TaskingSystemEnter();

  // We must be sure that all pending tasks are done
  TaskingSystemWaitAll();

  // Beyond this point, the worker threads are not doing anything
  GameEnd();
}


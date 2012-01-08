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

#include "game_frame.hpp"
#include "game_event.hpp"
#include "game_render.hpp"
#include "camera.hpp"
#include "sys/alloc.hpp"
#include "sys/windowing.hpp"

namespace pf
{
  GameFrame::GameFrame(GameFrame &previous) {
    this->cam = PF_NEW(FPSCamera, *previous.cam);
    this->event = PF_NEW(InputControl, *previous.event);
  }

  GameFrame::GameFrame(int w, int h) {
    this->cam = PF_NEW(FPSCamera);
    this->event = PF_NEW(InputControl, w, h);
  }

  TaskGameFrame::TaskGameFrame(GameFrame &previous_) : previous(&previous_) {}

  Task *TaskGameFrame::run(void)
  {
    // User pressed ESCAPE
    if (UNLIKELY(previous->event->getKey(27) == true)) {
      TaskingSystemInterruptMain();
      return NULL;
    }

    // Generate the current frame tasks
    GameFrame *current = PF_NEW(GameFrame, *previous);
    Task *eventTask = PF_NEW(TaskEvent, *current->event);
    Task *cameraTask = PF_NEW(TaskCamera, *current->cam, *current->event);
    Task *renderTask = PF_NEW(TaskGameRender, *current->cam, *current->event);
    eventTask->starts(cameraTask);
    cameraTask->starts(renderTask);
    renderTask->ends(this);
    cameraTask->scheduled();
    eventTask->scheduled();
    renderTask->scheduled();

    // Spawn the next frame. Right now there is no overlapping
    TaskGameFrame *next = PF_NEW(TaskGameFrame, *current);
    this->starts(next);
    next->scheduled();

    return NULL;
  }
} /* namespace pf */


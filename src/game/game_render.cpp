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

#include "camera.hpp"
#include "game_render.hpp"
#include "game_event.hpp"
#include "renderer/renderer_context.hpp"

namespace pf
{
  extern RnObj renderObj;
  extern RnContext renderer;

  TaskGameRender::TaskGameRender(FPSCamera &cam, InputEvent &event) :
    TaskMain("TaskGameRender"), cam(&cam), event(&event)
  {}

  Task* TaskGameRender::run(void)
  {
    RnTask displayTask = NULL;
    RnDisplayList list = rnDisplayListNew(renderer);
    RnFrame frame = rnFrameNew(renderer);
    rnDisplayListAddObj(list, renderObj);
    rnDisplayListCompile(list);
    rnFrameSetDisplayList(frame, list);
    rnFrameSetCamera(frame, &cam->org.x, &cam->up.x, &cam->view.x, cam->fov, cam->ratio);
    rnFrameSetScreenDimension(frame, event->w, event->h);
    rnFrameCompile(frame);
    displayTask = rnFrameDisplay(frame);
    rnFrameDelete(frame);
    rnDisplayListDelete(list);
    displayTask->ends(this);
    return displayTask;
  }

} /* namespace pf */


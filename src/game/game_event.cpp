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

#include "game_event.hpp"
#include "sys/mutex.hpp"
#include "sys/logging.hpp"
#include "sys/input_control.hpp"

namespace pf
{
  static double t = 0.f;
  static uint64 frameNum = 0;

  TaskEvent::TaskEvent(InputControl &current) :
    TaskMain("TaskEvent"), current(&current), cloneRoot(NULL)
  {}

  // static double prevT0 = 0.;
  Task *TaskEvent::run(void)
  {
    if (UNLIKELY(frameNum++ == 0))
      t = getSeconds();
    else if (frameNum % 256 == 0) {
      const double t0 = getSeconds();
      printf("\rfps %f frame/s", 256. / (t0 - t));
      fflush(stdout);
      t = t0;
    }
    current->processEvents();
    return NULL;
  }
}


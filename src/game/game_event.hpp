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

#ifndef __PF_GAME_EVENT_HPP__
#define __PF_GAME_EVENT_HPP__

#include "sys/tasking.hpp"
#include "sys/tasking_utility.hpp"

namespace pf
{
  class InputControl;

  /*! Record all events (keyboard, mouse, resizes) */
  class TaskEvent : public TaskMain
  {
  public:
    /*! Previous events are used to get delta values */
    TaskEvent(InputControl &current);

    /*! Register all events relatively to the previous ones (if any) */
    virtual Task *run(void);

    Task *clone(void) {
      TaskEvent *task = PF_NEW(TaskEvent, *this->current);
      task->cloneRoot = this->cloneRoot ? this->cloneRoot : this;
      task->ends(task->cloneRoot.ptr);
      return task;
    }

    Ref<InputControl> current;
    Ref<Task> cloneRoot;
    PF_CLASS(TaskEvent);
  };
} /* namespace pf */

#endif /* __PF_GAME_EVENT_HPP__ */


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

#ifndef __PF_FRAME_HPP__
#define __PF_FRAME_HPP__

#include "sys/tasking.hpp"

namespace pf {

  class FlyCamera;
  class InputEvent;

  /*! Stores the data required for the frame */
  class Frame : public RefCount
  {
  public:
    Frame(Frame *previous = NULL);
    Ref<FlyCamera> cam;    //!< Camera for this frame
    Ref<InputEvent> event; //!< Input as captured for this frame
  };

  /*! Responsible to handle the complete frame */
  class TaskFrame : public Task
  {
  public:
    /*! Frames are chained one after other */
    TaskFrame(Frame *previous = NULL);
    /*! Will basically spawn all the other tasks */
    virtual Task *run(void);
    Ref<Frame> previous;
  };
}

#endif /* __PF_FRAME_HPP__ */


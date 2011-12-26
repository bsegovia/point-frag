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
  /*! Contains the fields processed by the event handler */
  class InputEvent : public RefCount, public NonCopyable
  {
  public:
    InputEvent(const InputEvent &previous);
    InputEvent(int w, int h);
    INLINE bool getKey(int32 key) const {
      const int entry = key / 32; // which bitfield? (int32 == 32 bits)
      const int bit = key % 32;   // which bit in the bitfield?
      return (this->keys[entry] & (1u << bit)) != 0;
    }
    INLINE void upKey(int32 key) {
      const int entry = key / 32, bit = key % 32;
      this->keys[entry] &= ~(1u << bit);
    }
    INLINE void downKey(int32 key) {
      const int entry = key / 32, bit = key % 32;
      this->keys[entry] |=  (1u << bit);
    }
    enum { MAX_KEYS = 256u };
    enum { KEY_ARRAY_SIZE = MAX_KEYS / sizeof(int32) };
    double time;                //!< Current time when capturing the events
    double dt;                  //!< Delta from the previous frame
    int mouseX, mouseY;         //!< Absolute position of the mouse
    int mouseXRel, mouseYRel;   //!< Position of the mouse (relative to previous frame)
    int w, h;                   //!< Dimension of the window (if changed)
    char isResized:1;           //!< True if the user resized the window
  private:
    int32 keys[KEY_ARRAY_SIZE]; //!< Bitfield saying if key is pressed?
    PF_CLASS(InputEvent); // Declare custom new / delete operators
  };

  /*! Record all events (keyboard, mouse, resizes) */
  class TaskEvent : public TaskMain
  {
  public:
    /*! Previous events are used to get delta values */
    TaskEvent(InputEvent &current, InputEvent &previous);

    /*! Register all events relatively to the previous ones (if any) */
    virtual Task *run(void);

    /*! Mainly to initialize the underlying API */
    static void init(void);
    static void keyDown(unsigned char key, int x, int y);
    static void keyUp(unsigned char key, int x, int y);
    static void mouse(int button, int state, int x, int y);
    static void reshape(int w, int h);
    static void entry(int state);
    static void motion(int x, int y);

    Task *clone(void) {
      TaskEvent *task = PF_NEW(TaskEvent, *this->current, *this->previous);
      task->cloneRoot = this->cloneRoot ? this->cloneRoot : this;
      task->ends(task->cloneRoot.ptr);
      return task;
    }

    /*! Both events are needed to handle deltas */
    Ref<InputEvent> current;
    Ref<InputEvent> previous;
    Ref<Task> cloneRoot;
  };
} /* namespace pf */

#endif /* __PF_GAME_EVENT_HPP__ */


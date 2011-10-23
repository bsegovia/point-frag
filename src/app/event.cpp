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

#include "event.hpp"
#include "sys/mutex.hpp"
#include <GL/freeglut.h>

namespace pf
{
  /*! Only way to to use GLUT is to use this global */
  static TaskEvent *taskEvent = NULL;

  InputEvent::InputEvent(InputEvent *previous) {
    this->mouseXRel = this->mouseYRel = 0;
    this->isMouseInit = this->isResized = 0;
    if (previous) {
      this->w = previous->w;
      this->h = previous->h;
    } else
      this->w = this->h = 0;
    for (intptr_t i = 0; i < KEY_ARRAY_SIZE; ++i)
      this->keys[i] = previous->keys[i];
  }

  TaskEvent::TaskEvent(Ref<InputEvent> current,
                       Ref<InputEvent> previous) : Task("TaskEvent") {
    assert(current);
    this->setAffinity(PF_TASK_MAIN_THREAD);
  }

  void TaskEvent::keyDown(unsigned char key, int x, int y) {
    taskEvent->current->setKey(1);
  }

  void TaskEvent::keyUp(unsigned char key, int x, int y) {
    taskEvent->current->setKey(0);
  }

  void TaskEvent::mouse(int button, int state, int x, int y) { }

  void TaskEvent::reshape(int w, int h) {
    taskEvent->current->w = w;
    taskEvent->current->h = h;
    taskEvent->current->isResized = 1;
  }

  void TaskEvent::entry(int state) {
    if (state == GLUT_LEFT) {
      if (taskEvent->previous)
        glutWarpPointer(taskEvent->current->w/2,taskEvent->current->h/2);
      taskEvent->current->isMouseInit = 0;
    }
  }

  void TaskEvent::motion(int x, int y) {
    if (taskEvent->previous == false ||
        taskEvent->previous->isMouseInit == false) {
      taskEvent->current->mouseX = x;
      taskEvent->current->mouseY = y;
      taskEvent->current->mouseYRel = 0;
      taskEvent->current->mouseXRel = 0;
      taskEvent->current->isMouseInit = true;
    } else {
      taskEvent->current->mouseXRel = x - taskEvent->previous->mouseX;
      taskEvent->current->mouseYRel = y - taskEvent->previous->mouseY;
      taskEvent->current->mouseX = x;
      taskEvent->current->mouseY = y;
    }
  }

  void TaskEvent::init(void) {
    glutReshapeFunc(TaskEvent::reshape);
    glutEntryFunc(TaskEvent::entry);
    glutMouseFunc(TaskEvent::mouse);
    glutMotionFunc(TaskEvent::motion);
    glutPassiveMotionFunc(TaskEvent::motion);
    glutKeyboardFunc(TaskEvent::keyDown);
    glutKeyboardUpFunc(TaskEvent::keyUp);
  }

  Task *TaskEvent::run(void) {
    if (UNLIKELY(taskEvent == NULL)) TaskEvent::init();
    taskEvent = this;
    this->current->time = getSeconds();
    this->current->dt = this->current->time - this->previous->time;
    glutMainLoopEvent();
    return NULL;
  }
}


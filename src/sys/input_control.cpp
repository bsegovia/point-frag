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

#include "input_control.hpp"
#include "mutex.hpp"
#include <GL/freeglut.h>

#include "logging.hpp"
namespace pf
{
  ///////////////////////////////////////////////////////////////////////////
  /// GLUT callbacks
  ///////////////////////////////////////////////////////////////////////////

  /*! Only way to to use GLUT is to use this global */
  static InputControl *inputControl = NULL;
  static MutexSys inputControlMutex;
  double lastTime = 0.f;
  static void keyDown(unsigned char key, int x, int y) {
    inputControl->downKey(key);
    inputControl->keyPressed.push_back(key);
  }
  static void keyUp(unsigned char key, int x, int y) {
    inputControl->upKey(key);
    inputControl->keyReleased.push_back(key);
  }
  static void specialKeyDown(int key, int x, int y) {
    inputControl->downKey(key + 0x80);
    inputControl->keyPressed.push_back(key + 0x80);
  }
  static void specialKeyUp(int key, int, int) {
    inputControl->upKey(key + 0x80);
    inputControl->keyReleased.push_back(key + 0x80);
  }
  static void mouse(int button, int state, int x, int y) { }
  static void entry(int state) { }
  static void reshape(int w, int h) {
    inputControl->w = w;
    inputControl->h = h;
    inputControl->isResized = 1;
  }
  static void motion(int x, int y) {
    inputControl->mouseXRel = x - inputControl->w / 2;
    inputControl->mouseYRel = y - inputControl->h / 2;
  }
  static void init(void) {
    glutReshapeFunc(reshape);
    glutEntryFunc(entry);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialUpFunc(specialKeyUp);
    glutSpecialFunc(specialKeyDown);
  }

  ///////////////////////////////////////////////////////////////////////////
  /// Input Control
  ///////////////////////////////////////////////////////////////////////////

  STATIC_ASSERT(PF_KEY_F1 == GLUT_KEY_F1 + 0x80);
  STATIC_ASSERT(PF_KEY_F2 == GLUT_KEY_F2 + 0x80);
  STATIC_ASSERT(PF_KEY_F3 == GLUT_KEY_F3 + 0x80);
  STATIC_ASSERT(PF_KEY_F4 == GLUT_KEY_F4 + 0x80);
  STATIC_ASSERT(PF_KEY_F5 == GLUT_KEY_F5 + 0x80);
  STATIC_ASSERT(PF_KEY_F6 == GLUT_KEY_F6 + 0x80);
  STATIC_ASSERT(PF_KEY_F7 == GLUT_KEY_F7 + 0x80);
  STATIC_ASSERT(PF_KEY_F8 == GLUT_KEY_F8 + 0x80);
  STATIC_ASSERT(PF_KEY_F9 == GLUT_KEY_F9 + 0x80);
  STATIC_ASSERT(PF_KEY_F10 == GLUT_KEY_F10 + 0x80);
  STATIC_ASSERT(PF_KEY_F11 == GLUT_KEY_F11 + 0x80);
  STATIC_ASSERT(PF_KEY_F12 == GLUT_KEY_F12 + 0x80);
  STATIC_ASSERT(PF_KEY_LEFT == GLUT_KEY_LEFT + 0x80);
  STATIC_ASSERT(PF_KEY_UP == GLUT_KEY_UP + 0x80);
  STATIC_ASSERT(PF_KEY_RIGHT == GLUT_KEY_RIGHT + 0x80);
  STATIC_ASSERT(PF_KEY_DOWN == GLUT_KEY_DOWN + 0x80);
  STATIC_ASSERT(PF_KEY_PAGE_UP == GLUT_KEY_PAGE_UP + 0x80);
  STATIC_ASSERT(PF_KEY_PAGE_DOWN == GLUT_KEY_PAGE_DOWN + 0x80);
  STATIC_ASSERT(PF_KEY_HOME == GLUT_KEY_HOME + 0x80);
  STATIC_ASSERT(PF_KEY_END == GLUT_KEY_END + 0x80);
  STATIC_ASSERT(PF_KEY_INSERT == GLUT_KEY_INSERT + 0x80);

  InputControl::InputControl(int w_, int h_) {
    this->mouseXRel = this->mouseYRel = 0;
    this->time = this->dt = 0.;
    this->w = w_;
    this->h = h_;
    this->isResized = 0;
    for (uint32 i = 0; i < KEY_ARRAY_SIZE; ++i) this->keys[i] = 0;
  }

  InputControl::InputControl(const InputControl &previous) {
    this->mouseXRel = this->mouseYRel = 0;
    this->isResized = 0;
    this->time = getSeconds();
    this->dt = 0.;
    this->w = previous.w;
    this->h = previous.h;
    for (uint32 i = 0; i < KEY_ARRAY_SIZE; ++i)
      this->keys[i] = previous.keys[i];
  }

  void InputControl::processEvents(void) {
    Lock<MutexSys> lock(inputControlMutex);
    if (UNLIKELY(inputControl == NULL)) {
      init();
      this->dt = inf;
      this->time = lastTime = getSeconds();
    } else {
      this->time = getSeconds();
      this->dt = this->time - lastTime;
      lastTime = this->time;
    }
    inputControl = this;
    glutMainLoopEvent();
    glutWarpPointer(this->w/2, this->h/2);
    const int w0 = glutGet(GLUT_WINDOW_WIDTH);
    const int h0 = glutGet(GLUT_WINDOW_HEIGHT);
    if (w0 != this->w || h0 != this->h) this->isResized = 1;
    this->w = w0;
    this->h = h0;
  }
} /* namespace pf */


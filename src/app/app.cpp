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

#include "frame.hpp"
#include "sys/alloc.hpp"
#include "sys/tasking.hpp"
#include "renderer/renderobj.hpp"
#include "renderer/renderer.hpp"

#include <GL/freeglut.h>

using namespace pf;

namespace pf {
  Renderer *renderer = NULL;
  Ref<RenderObj> renderObj = NULL;
}

void GameStart(int argc, char **argv)
{
  glutInitWindowSize(800, 600);
  glutInitWindowPosition(64, 64);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
  glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
  glutCreateWindow(argv[0]);
  ogl = renderer = PF_NEW(Renderer);
  renderObj = PF_NEW(RenderObj, *renderer, "f000.obj");
}

void GameEnd()
{
  renderObj = NULL;
  PF_DELETE(renderer);
  ogl = renderer = NULL;
}

int main(int argc, char **argv)
{
  MemDebuggerStart();
  TaskingSystemStart();
  GameStart(argc, argv);

  Task *frame = PF_NEW(TaskFrame, NULL);
  frame->scheduled();
  TaskingSystemEnter();

  GameEnd();
  TaskingSystemEnd();
  MemDebuggerDumpAlloc();
  return 0;
}


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
#include "models/obj.hpp"
#include "renderer/texture.hpp"
#include "renderer/renderer_obj.hpp"
#include "renderer/renderer.hpp"

#include <GL/freeglut.h>

namespace pf
{
  extern Renderer *renderer;
  extern Ref<RendererObj> renderObj;

#define OGL_NAME ((RendererDriver*)renderer->driver)

  TaskGameRender::TaskGameRender(FlyCamera &cam, InputEvent &event) :
    TaskMain("TaskGameRender"), cam(&cam), event(&event)
  {}

  Task* TaskGameRender::run(void)
  {
    // Transform matrix for the current point of view
    const mat4x4f MVP = cam->getMatrix();

    // Set the display viewport
    R_CALL (Viewport, 0, 0, event->w, event->h);
    R_CALL (Enable, GL_DEPTH_TEST);
    R_CALL (Disable, GL_CULL_FACE);
    R_CALL (CullFace, GL_FRONT);
    R_CALL (DepthFunc, GL_LEQUAL);

    // Clear color buffer with black
    R_CALL (ClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
    R_CALL (ClearDepth, 1.0f);
    R_CALL (Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Display the objects with their textures
    R_CALL (UseProgram, renderer->driver->diffuse.program);
    R_CALL (UniformMatrix4fv, renderer->driver->diffuse.uMVP, 1, GL_FALSE, &MVP[0][0]);
    renderObj->display();
    R_CALL (UseProgram, 0);

    // Display all the bounding boxes
    R_CALL (setMVP, MVP);
    R_CALL (displayBBox, renderObj->bbox, renderObj->grpNum);
    glutSwapBuffers();

    return NULL;
  }

#undef OGL_NAME

} /* namespace pf */


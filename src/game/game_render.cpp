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

#include "rt/rt_camera.hpp" // XXX to do frustum culling

namespace pf
{
  extern Renderer *renderer;
  extern Ref<RendererObj> renderObj;

#define OGL_NAME ((RendererDriver*)renderer->driver)

  TaskGameRender::TaskGameRender(FPSCamera &cam, InputEvent &event) :
    TaskMain("TaskGameRender"), cam(&cam), event(&event)
  {}

  INLINE vec3f rtCameraXfm(const RTCamera &cam, const vec3f &p)
  {
    const vec3f dir = normalize(p - cam.org);
    const float x = dot(dir, normalize(cam.xAxis));
    const float y = dot(dir, cam.zAxis);
    const float z = dot(dir, cam.view);
    return vec3f(x,y,z);
  }

  // XXX Quick and dirty test
  static void cullObj(RendererObj &renderObj, const FPSCamera &fpsCam)
  {
    const RTCamera cam(fpsCam.org, fpsCam.up, fpsCam.view, fpsCam.fov, fpsCam.ratio);
    RendererObj::Segment *sgmt = PF_NEW_ARRAY(RendererObj::Segment, renderObj.sgmtNum);
    uint32 curr = 0;
    for (uint32 i = 0; i < renderObj.sgmtNum; ++i) {
      const vec3f m = renderObj.sgmt[i].bbox.lower;
      const vec3f M = renderObj.sgmt[i].bbox.upper;
      const vec3f v0 = rtCameraXfm(cam, vec3f(m[0],m[1],m[2]));
      const vec3f v1 = rtCameraXfm(cam, vec3f(M[0],m[1],m[2]));
      const vec3f v2 = rtCameraXfm(cam, vec3f(m[0],M[1],m[2]));
      const vec3f v3 = rtCameraXfm(cam, vec3f(M[0],M[1],m[2]));
      const vec3f v4 = rtCameraXfm(cam, vec3f(m[0],m[1],M[2]));
      const vec3f v5 = rtCameraXfm(cam, vec3f(M[0],m[1],M[2]));
      const vec3f v6 = rtCameraXfm(cam, vec3f(m[0],M[1],M[2]));
      const vec3f v7 = rtCameraXfm(cam, vec3f(M[0],M[1],M[2]));
      const vec3f vmin = min(min(min(min(min(min(min(v0,v1),v2),v3),v4),v5),v6),v7);
      const vec3f vmax = max(max(max(max(max(max(max(v0,v1),v2),v3),v4),v5),v6),v7);

      //if (i == 100) 
      {
#if 0
      printf("\r%f %f %f %f %f %f %f %f %f",
          vmin.x, vmin.y, vmin.z, vmax.x, vmax.y, vmax.z, cam.ratio, cam.fov, cam.dist);
#endif
      const float c = cos(cam.fov * (float) M_PI / 360.f);
      if ((vmax.x < -cam.ratio * c) ||
          (vmax.y < -c) ||
          (vmax.z < 0.f) ||
          (vmin.x > cam.ratio * c) ||
          (vmin.y > c)) continue;

        sgmt[curr++] = renderObj.sgmt[i];
      }
    }
    renderObj.sgmt = sgmt;
    renderObj.sgmtNum = curr;
  }

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

    // XXX
    RendererObj::Segment *saved  = renderObj->sgmt;
    const uint32 savedNum = renderObj->sgmtNum;
    cullObj(*renderObj, *cam);
    renderObj->display();
    R_CALL (UseProgram, 0);

    // Display all the bounding boxes
    R_CALL (setMVP, MVP);
    BBox3f *bbox = PF_NEW_ARRAY(BBox3f, renderObj->sgmtNum);
    for (size_t i = 0; i < renderObj->sgmtNum; ++i)
      bbox[i] = renderObj->sgmt[i].bbox;
    R_CALL (displayBBox, bbox, renderObj->sgmtNum);
    PF_SAFE_DELETE_ARRAY(bbox);
    glutSwapBuffers();

    // XXX
    PF_DELETE_ARRAY(renderObj->sgmt);
    renderObj->sgmt = saved;
    renderObj->sgmtNum = savedNum;

    return NULL;
  }

#undef OGL_NAME

} /* namespace pf */


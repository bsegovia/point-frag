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
#include "renderer/renderer_driver.hpp"
#include "image/stb_image.hpp"

#include "sys/logging.hpp"
#include "rt/rt_camera.hpp" // XXX to do frustum culling
#include "rt/bvh2.hpp" // XXX to do frustum culling
#include "rt/bvh2_traverser.hpp" // XXX to do frustum culling

#if 1

#include "rt/rt_triangle.hpp" // XXX to do frustum culling
#include "rt/rt_camera.hpp" // XXX to do frustum culling
#include "renderer/hiz.hpp" // XXX HiZ
#include <cstring>

namespace pf
{
  extern Ref<RendererObj> renderObj; //!< Real world should come later
  Ref<BVH2<RTTriangle>> bvh = NULL; // XXX -> HiZ

#define OGL_NAME ((RendererDriver*)renderObj->renderer.driver)

  TaskGameRender::TaskGameRender(FPSCamera &cam, InputEvent &event) :
    TaskMain("TaskGameRender"), cam(&cam), event(&event)
  {}

  INLINE vec3f rtCameraXfm(const RTCamera &cam, const vec3f &p)
  {
    const vec3f dir = p - cam.org;
    const float z = dot(p - cam.org, cam.view);
    const float x = dot(dir, normalize(cam.xAxis)) / z;
    const float y = dot(dir, cam.zAxis) / z;

    return vec3f(x,y,z);
  }

  static RendererObj::Segment *cullSegment = NULL;
  static uint32 cullNum = 0;

  // XXX Quick and dirty test
  static void cullObj(RendererObj &renderObj, const FPSCamera &fpsCam, const InputEvent &event)
  {
    // Boiler plate for the HiZ
   // Ref<HiZ> hiz = PF_NEW(HiZ, 1024, 1024);
    //Ref<HiZ> hiz = PF_NEW(HiZ, 128, 128);
    //Ref<HiZ> hiz = PF_NEW(HiZ, 256, 256);
    //Ref<HiZ> hiz = PF_NEW(HiZ, 512, 512);
    Ref<HiZ> hiz = PF_NEW(HiZ, 64, 64);
    Ref<Intersector> intersector = PF_NEW(BVH2Traverser<RTTriangle>, bvh);
    const RTCamera cam(fpsCam.org, fpsCam.up, fpsCam.view, fpsCam.fov, fpsCam.ratio);
    Ref<Task> task = hiz->rayTrace(cam, intersector);
    task->waitForCompletion();

    RendererObj::Segment *sgmt = PF_NEW_ARRAY(RendererObj::Segment, renderObj.sgmtNum);
    uint32 curr = 0;
    for (uint32 i = 0; i < renderObj.sgmtNum; ++i) {
      vec3f pmin = renderObj.sgmt[i].bbox.lower;
      vec3f pmax = renderObj.sgmt[i].bbox.upper;
      for (int j = 0; j < 3; ++j) {
        pmin[j] -= 0.f;
        pmax[j] += 0.f;
      }

      const vec3f m = pmin;
      const vec3f M = pmax;
      const vec3f v0 = rtCameraXfm(cam, vec3f(m[0],m[1],m[2]));
      const vec3f v1 = rtCameraXfm(cam, vec3f(M[0],m[1],m[2]));
      const vec3f v2 = rtCameraXfm(cam, vec3f(m[0],M[1],m[2]));
      const vec3f v3 = rtCameraXfm(cam, vec3f(M[0],M[1],m[2]));
      const vec3f v4 = rtCameraXfm(cam, vec3f(m[0],m[1],M[2]));
      const vec3f v5 = rtCameraXfm(cam, vec3f(M[0],m[1],M[2]));
      const vec3f v6 = rtCameraXfm(cam, vec3f(m[0],M[1],M[2]));
      const vec3f v7 = rtCameraXfm(cam, vec3f(M[0],M[1],M[2]));
      vec3f vmin = min(min(min(min(min(min(min(v0,v1),v2),v3),v4),v5),v6),v7);
      vec3f vmax = max(max(max(max(max(max(max(v0,v1),v2),v3),v4),v5),v6),v7);

      //if (i == 160)
      if (1)
      {
#if 0
      printf("\r%f %f %f %f %f %f %f %f %f",
          vmin.x, vmin.y, vmin.z, vmax.x, vmax.y, vmax.z, cam.ratio, cam.fov, cam.dist);
#endif
      // frustum culling first
      const float c = tan(cam.fov * float(pi) / 360.f);
      vmax.x /= cam.ratio * c;
      vmax.y /= c;
      vmin.x /= cam.ratio * c;
      vmin.y /= c;
      if ((vmax.x < -1.f) ||
          (vmax.y < -1.f) ||
          (vmax.z < 0.f) ||
          (vmin.x > 1.f) ||
          (vmin.y > 1.f)) continue;

      // compute the location of the closest point
      vec3f closest;
      closest.x = (cam.org.x < pmin.x) ? pmin.x : (cam.org.x > pmax.x) ? pmax.x : cam.org.x;
      closest.y = (cam.org.y < pmin.y) ? pmin.y : (cam.org.y > pmax.y) ? pmax.y : cam.org.y;
      closest.z = (cam.org.z < pmin.z) ? pmin.z : (cam.org.z > pmax.z) ? pmax.z : cam.org.z;
      const float z = dot(-cam.org + closest, cam.view);

      // Now the z test with HiZ
      vmax.x = vmax.x * 0.5f + 0.5f;
      vmax.y = vmax.y * 0.5f + 0.5f;
      vmin.x = vmin.x * 0.5f + 0.5f;
      vmin.y = vmin.y * 0.5f + 0.5f;
      const vec2i dim(hiz->width, hiz->height);
      const vec2f dimf(float(hiz->width), float(hiz->height));
      const vec2i vmini = min(dim-vec2i(one),
        max(vec2i(zero), vec2i(int(dimf.x * vmin.x), int(dimf.y * vmin.y))));
      const vec2i vmaxi = min(dim-vec2i(one),
        max(vec2i(zero), vec2i(one) + vec2i(int(dimf.x * vmax.x), int(dimf.y * vmax.y))));

      // traverse all touched tiles
      bool visible = false;
      const vec2i minTile = vmini / vec2i(HiZ::Tile::width, HiZ::Tile::height);
      const vec2i maxTile = vmaxi / vec2i(HiZ::Tile::width, HiZ::Tile::height);
      for (int32 tileX = minTile.x; tileX <= maxTile.x; ++tileX) {
        for (int32 tileY = minTile.y; tileY <= maxTile.y; ++tileY) {
          const int32 tileID = tileX + tileY * hiz->tileXNum;
          assert(tileID < int32(hiz->tileNum));
          const HiZ::Tile &tile = hiz->tiles[tileID];
          if (z > tile.zmax && z /  tile.zmax > 1.05f) {
            continue;
          }
          visible = true;
        }
      }

      //if (!visible) printf("%i ", i);
      if (visible)
      sgmt[curr++] = renderObj.sgmt[i];
      }
    }
    if (event.getKey('l')) {
      if (cullSegment) PF_DELETE_ARRAY(cullSegment);
      cullSegment = PF_NEW_ARRAY(RendererObj::Segment, curr);
      std::memcpy(cullSegment, sgmt, curr * sizeof(RendererObj::Segment));
      cullNum = curr;
    }
    if (event.getKey('k') && cullSegment) {
      PF_DELETE_ARRAY(sgmt);
      sgmt = PF_NEW_ARRAY(RendererObj::Segment, cullNum);
      std::memcpy(sgmt, cullSegment, cullNum * sizeof(RendererObj::Segment));
      curr = cullNum;
    }
    // XXX for HiZ
    if (event.getKey('p'))
    {
      // Output the complete HiZ
      uint8 *rgba = PF_NEW_ARRAY(uint8, 4 * hiz->width * hiz->height);
      hiz->greyRGBA(&rgba);
      stbi_write_tga("hiz.tga", hiz->width, hiz->height, 4, rgba);
      PF_DELETE_ARRAY(rgba);

      // Output the minimum z values
      rgba = PF_NEW_ARRAY(uint8, 4 * hiz->tileXNum * hiz->tileYNum);
      hiz->greyMinRGBA(&rgba);
      stbi_write_tga("hiz_min.tga",  hiz->tileXNum, hiz->tileYNum, 4, rgba);

      // Output the maximum z values
      hiz->greyMaxRGBA(&rgba);
      stbi_write_tga("hiz_max.tga",  hiz->tileXNum, hiz->tileYNum, 4, rgba);
      PF_DELETE_ARRAY(rgba);
    }

    renderObj.sgmt = sgmt;
    renderObj.sgmtNum = curr;
  }

  Task* TaskGameRender::run(void)
  {
    // Transform matrix for the current point of view
    const mat4x4f MVP = cam->getMatrix();
    Renderer *renderer = &renderObj->renderer;

    // Set the display viewport
    R_CALL (Viewport, 0, 0, event->w, event->h);
    R_CALL (Enable, GL_DEPTH_TEST);
    R_CALL (Enable, GL_CULL_FACE);
    R_CALL (CullFace, GL_BACK);
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
    cullObj(*renderObj, *cam, *event);
    renderObj->display();
    R_CALL (UseProgram, 0);

    // Display all the bounding boxes
    R_CALL (setMVP, MVP);
    BBox3f *bbox = PF_NEW_ARRAY(BBox3f, renderObj->sgmtNum);
    for (size_t i = 0; i < renderObj->sgmtNum; ++i)
      bbox[i] = renderObj->sgmt[i].bbox;
    //R_CALL (displayBBox, bbox, renderObj->sgmtNum);
    PF_SAFE_DELETE_ARRAY(bbox);
    R_CALL (swapBuffers);

    // XXX
    PF_DELETE_ARRAY(renderObj->sgmt);
    renderObj->sgmt = saved;
    renderObj->sgmtNum = savedNum;

    return NULL;
  }

#undef OGL_NAME

} /* namespace pf */

#endif
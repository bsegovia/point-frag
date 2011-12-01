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
#include "renderer/renderer_segment.hpp"
#include "image/stb_image.hpp"
#include "sys/logging.hpp"

// To do HIZ culling
#include "rt/rt_camera.hpp"
#include "rt/bvh2.hpp"
#include "rt/bvh2_traverser.hpp"
#include "rt/rt_triangle.hpp"
#include "rt/rt_camera.hpp"
#include "renderer/hiz.hpp"
#include <cstring>

namespace pf
{
  extern Ref<RendererObj> renderObj; //!< Real world should come later
  Ref< BVH2<RTTriangle> > bvh = NULL;  // XXX -> HiZ
  Ref<RendererObj> renderObj = NULL;

#define OGL_NAME ((RendererDriver*)renderObj->renderer.driver)

  TaskGameRender::TaskGameRender(FPSCamera &cam, InputEvent &event) :
    TaskMain("TaskGameRender"), cam(&cam), event(&event)
  {}

  /*! SIMD optimized structure to perform frustum and HiZ culling */
  struct PerspectiveFrustum
  {
    /*! Setup all values properly */
    PerspectiveFrustum(const RTCamera &cam, Ref<HiZ> hiz);
    /*! Cull a segment */
    INLINE bool isVisible(const RendererSegment &sgmt);
  private:
    ssef  org_aos;    //!< Origin in array of struct format
    ssef  view_aos;   //!< View vector in array of struct
    sse3f org;        //!< Splat view origin
    sse3f view;       //!< View direction (First axis)
    sse3f xAxis;      //!< Second axis dot(view,xAxis) == 0
    sse3f zAxis;      //!< Third axis  dot(xAxis, zAxis) == 0
    ssef xMaxSinAngle;//!< Maximum sine value along X axis
    ssef yMaxSinAngle;//!< Maximum sine value along Z axis
    ssef xMaxInvTanAngle;//!< Maximum tangent value along X axis
    ssef yMaxInvTanAngle;//!< Maximum tangent value along Z axis
    ssef windowing;      //!< To scale the position in {w,h}
    ssef hizExtent;      //!< Maximum extent in the HiZ {w-1,h-1}
    Ref<HiZ> hiz;        //!< To perform Z test
  };

  PerspectiveFrustum::PerspectiveFrustum(const RTCamera &cam, Ref<HiZ> hiz)
    : hiz(hiz)
  {
    this->org_aos  = ssef(cam.org.x, cam.org.y, cam.org.z, 0.f);
    this->view_aos = ssef(cam.view.x, cam.view.y, cam.view.z, 0.f);
    this->org   = sse3f(cam.org.x, cam.org.y, cam.org.z);
    this->view  = sse3f(cam.view.x, cam.view.y, cam.view.z);
    this->xAxis = sse3f(cam.xAxis.x, cam.xAxis.y, cam.xAxis.z);
    this->zAxis = sse3f(cam.zAxis.x, cam.zAxis.y, cam.zAxis.z);
    this->yMaxSinAngle = sin(cam.fov * float(pi) / 360.f);
    this->xMaxSinAngle = cam.ratio * this->yMaxSinAngle;
    this->yMaxInvTanAngle = 1.f / tan(cam.fov * float(pi) / 360.f);
    this->xMaxInvTanAngle = this->yMaxInvTanAngle / cam.ratio;
    this->xAxis = normalize(this->xAxis);
    this->zAxis = normalize(this->zAxis);
    this->windowing = ssef(float(hiz->tileXNum) * 0.5f,
                           float(hiz->tileXNum) * 0.5f,
                           float(hiz->tileYNum) * 0.5f,
                           float(hiz->tileYNum) * 0.5f);
    this->hizExtent = ssef(float(hiz->tileXNum-1),
                           float(hiz->tileXNum-1),
                           float(hiz->tileYNum-1),
                           float(hiz->tileYNum-1));
  }

// Do we want to use the HiZ buffer?
#define HIZ_USE_ZBUFFER 1

// Growing the AABB allows us to use really small HiZ buffer with no visible
// artifacts. Should be a run-time value
#define HIZ_GROW_AABB 1

  INLINE bool PerspectiveFrustum::isVisible(const RendererSegment &sgmt)
  {
#if HIZ_GROW_AABB
    const ssef lower = ssef::uload(&sgmt.bbox.lower.x) - ssef(one);
    const ssef upper = ssef::uload(&sgmt.bbox.upper.x) + ssef(one);
#else
    const ssef lower = ssef::uload(&sgmt.bbox.lower.x);
    const ssef upper = ssef::uload(&sgmt.bbox.upper.x);
#endif /* HIZ_GROW_AABB */

    const ssef x0 = lower.xxxx();
    const ssef x1 = upper.xxxx();
    const ssef y0z0y1z1 = shuffle<1,2,1,2>(lower, upper);
    const ssef y = y0z0y1z1.xzxz();
    const ssef z = y0z0y1z1.yyww();
    const ssef dir_x0 = x0 - org.x;
    const ssef dir_x1 = x1 - org.x;
    const ssef dir_y  = y  - org.y;
    const ssef dir_z  = z  - org.z;
    const ssef norm0 = sqrt(dir_x0*dir_x0 + dir_y*dir_y + dir_z*dir_z);
    const ssef norm1 = sqrt(dir_x1*dir_x1 + dir_y*dir_y + dir_z*dir_z);

    // Compute view direction angles (sine of angle more precisely) and depth
    // in camera space
    const ssef sinx0 = dir_x0*xAxis.x + dir_y*xAxis.y + dir_z*xAxis.z;
    const ssef sinx1 = dir_x1*xAxis.x + dir_y*xAxis.y + dir_z*xAxis.z;
    const ssef siny0 = dir_x0*zAxis.x + dir_y*zAxis.y + dir_z*zAxis.z;
    const ssef siny1 = dir_x1*zAxis.x + dir_y*zAxis.y + dir_z*zAxis.z;

    // Culled by left and right planes?
    const sseb sinxb0 = sinx0 > xMaxSinAngle * norm0;
    const sseb sinxb1 = sinx1 > xMaxSinAngle * norm1;
    const sseb sinxb0m = sinx0 < -xMaxSinAngle * norm0;
    const sseb sinxb1m = sinx1 < -xMaxSinAngle * norm1;
    if (movemask(sinxb0 & sinxb1) == 0xf) return false;
    if (movemask(sinxb0m & sinxb1m) == 0xf) return false;

    // Culled by top and bottom planes?
    const sseb sinyb0 = siny0 > yMaxSinAngle * norm0;
    const sseb sinyb1 = siny1 > yMaxSinAngle * norm1;
    const sseb sinyb0m = siny0 < -yMaxSinAngle * norm0;
    const sseb sinyb1m = siny1 < -yMaxSinAngle * norm1;
    if (movemask(sinyb0 & sinyb1) == 0xf) return false;
    if (movemask(sinyb0m & sinyb1m) == 0xf) return false;

    // Culled by near plane?
    const ssef z0 = dir_x0*view.x + dir_y*view.y + dir_z*view.z;
    const ssef z1 = dir_x1*view.x + dir_y*view.y + dir_z*view.z;
    if (movemask(z0 & z1) == 0xf) return false;

#if HIZ_USE_ZBUFFER
    // The bounding box is completely in front of us. We go to perspective space
    // and we use the HiZ to cull the box
    if (movemask(z0 | z1) == 0x0) {
      // We first consider the closest point on the bounding box and compute the
      // distance from it
      const ssef closest = min(max(lower,org_aos),upper);
      const ssef d = (closest - org_aos) * view_aos;
      const float zmin = extract<0>(d + d.yyyy() + d.zzzz());

      // Compute the position in perspective space and project
      const ssef _z0 = rcp(z0);
      const ssef _z1 = rcp(z1);
      const ssef x0p = sinx0 * _z0 * xMaxInvTanAngle;
      const ssef x1p = sinx1 * _z1 * xMaxInvTanAngle;
      const ssef y0p = siny0 * _z0 * yMaxInvTanAngle;
      const ssef y1p = siny1 * _z1 * yMaxInvTanAngle;

      // Screen space min / max positions
      const ssef xpm = reduce_min(min(x0p, x1p));
      const ssef xpM = reduce_max(max(x0p, x1p));
      const ssef ypm = reduce_min(min(y0p, y1p));
      const ssef ypM = reduce_max(max(y0p, y1p));
      const ssef xmM = unpacklo(xpm, xpM); // {x_min, x_max, x_min, x_max}
      const ssef ymM = unpacklo(ypm, ypM); // {y_min, y_max, y_min, y_max}
      const ssef mM  = movelh(xmM, ymM);   // {x_min, x_max, y_min, y_max}
      const ssef mMs = windowing + mM * windowing;

      // Clamp and convert to integer
      const ssei mMi = truncate(min(max(mMs, ssef(zero)), hizExtent));

      // Traverse the HiZ buffer
      const vec2i tileMin(mMi[0], mMi[2]);
      const vec2i tileMax(mMi[1], mMi[3]);
      uint32 leftID = tileMin.x + tileMin.y * hiz->tileXNum;
      for (int32 tileY = tileMin.y; tileY <= tileMax.y; ++tileY) {
        uint32 tileID = leftID;
        for (int32 tileX = tileMin.x; tileX <= tileMax.x; ++tileX, ++tileID) {
          assert(tileID < hiz->tileNum);
          const HiZ::Tile &tile = hiz->tiles[tileID];
          if (zmin > tile.zmax)
            continue;
          else
            return true;
        }
        leftID += hiz->tileXNum;
      }
      return false;
    } else
      return true;
#else
    return true;
#endif /* HIZ_USE_ZBUFFER */
  }

  // XXX Debug code to see the actual effect of culling. Press 'l' to save the
  // displayed boxes and 'k' to display these boxes later. There will be a leak
  // here since the last allocated cullSegment will not be allocated.
  static RendererSegment *cullSegment = NULL;
  static uint32 cullNum = 0;

  /*! Perform the HiZ culling (Frustum + Z) on the given segments */
  static void HiZCull(RendererObj &renderObj,
                      const FPSCamera &fpsCam,
                      const InputEvent &event)
  {
    // Compute the HiZ buffer
    Ref<HiZ> hiz = PF_NEW(HiZ, 256, 128);
    Ref<Intersector> intersector = PF_NEW(BVH2Traverser<RTTriangle>, bvh);
    const RTCamera cam(fpsCam.org, fpsCam.up, fpsCam.view, fpsCam.fov, fpsCam.ratio);
    Ref<Task> task = hiz->rayTrace(cam, intersector);
    task->waitForCompletion();

    // Now cull the segments
    PerspectiveFrustum fr(cam, hiz);
    RendererSegment *segments = PF_NEW_ARRAY(RendererSegment, renderObj.segmentNum);
    uint32 curr = 0;
    for (uint32 i = 0; i < renderObj.segmentNum; ++i)
      if (fr.isVisible(renderObj.segments[i]))
        segments[curr++] = renderObj.segments[i];

    // XXX next is debug code
    if (event.getKey('l')) {
      if (cullSegment) PF_DELETE_ARRAY(cullSegment);
      cullSegment = PF_NEW_ARRAY(RendererSegment, curr);
      std::memcpy(cullSegment, segments, curr * sizeof(RendererSegment));
      cullNum = curr;
    }

    if (event.getKey('k') && cullSegment) {
      PF_DELETE_ARRAY(segments);
      segments = PF_NEW_ARRAY(RendererSegment, cullNum);
      std::memcpy(segments, cullSegment, cullNum * sizeof(RendererSegment));
      curr = cullNum;
    }

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

    renderObj.segments = segments;
    renderObj.segmentNum = curr;
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

    // XXX A bit ugly. We saved the boxes since we are going to override the
    // array in the culling functions
    RendererSegment *saved  = renderObj->segments;
    const uint32 savedNum = renderObj->segmentNum;
    HiZCull(*renderObj, *cam, *event);
    renderObj->display();
    R_CALL (UseProgram, 0);

    // Display all the bounding boxes
    R_CALL (setMVP, MVP);
    BBox3f *bbox = PF_NEW_ARRAY(BBox3f, renderObj->segmentNum);
    for (size_t i = 0; i < renderObj->segmentNum; ++i)
      bbox[i] = renderObj->segments[i].bbox;
    R_CALL (displayBBox, bbox, renderObj->segmentNum);
    PF_SAFE_DELETE_ARRAY(bbox);
    R_CALL(swapBuffers);

    // XXX Reset back the initial segments
    PF_DELETE_ARRAY(renderObj->segments);
    renderObj->segments = saved;
    renderObj->segmentNum = savedNum;

    return NULL;
  }

#undef OGL_NAME

} /* namespace pf */


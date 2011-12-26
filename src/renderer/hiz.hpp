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

#ifndef __PF_HIZ_HPP__
#define __PF_HIZ_HPP__

#include "renderer_segment.hpp"
#include "rt/ray_packet.hpp"
#include "simd/ssef.hpp"
#include "sys/ref.hpp"
#include "sys/tasking.hpp"

namespace pf
{
  class RTCamera;    // To generate the rays
  class Task;        // A task is generated for the ray tracing part
  class Intersector; // We use an intersector to compute the depth buffer

  /*! This structure will allow us to perform software culling of the objects.
   *  We build a small z buffer for the current point of view and we use it to
   *  compare the depth of bounding rectangles and cull draw calls. Since we
   *  use ray packets and we want to have a hierarchy over the depth buffer, we
   *  simply tile the depth buffer with RayPacket::width x RayPacket::height
   *  tiles. We reference count it since it is possibly filled by several
   *  threads
   */
  struct HiZ : public RefCount, public NonCopyable
  {
    /*! width and height will be aligned on Tile::width and Tile::height if
     *  required
     */
    HiZ(uint32 width = 256u, uint32 height = 256u);

    /*! Free the allocated data */
    ~HiZ(void);

    /*! HiZ is made of tiles. Each tile is traced by _one_ ray packet */
    struct Tile
    {
      static const uint32 width = RayPacket::width;
      static const uint32 height = RayPacket::height;
      static const uint32 pixelNum = width * height;
      static const uint32 chunkNum = pixelNum / ssef::CHANNEL_NUM;
      ssef z[chunkNum]; //!< Depths per pixel (layout in struct-of-array)
      float zmin;       //!< Minimum depth in the tile
      float zmax;       //!< Maximum depth in the tile
    };

    /*! Fill the depth buffer with the given point of view. We will not hold a
     *  reference on the camera but we need to keep the intersector alive
     *  after the function call. Returns the ray tracing task reference
     */
    Ref<Task> rayTrace(const RTCamera &cam, Ref<Intersector> intersector);

    /*! Output a RGBA image into *pixels that shows the final result in grey
     *  colors. No sync is done, so the user is responsible for it. Dimension of
     *  the image is width x height
     */
    void greyRGBA(uint8 **rgba) const;

    /*! Idem but output the min buffer. Dimension is tileXNum x tileYNum */
    void greyMinRGBA(uint8 **rgba) const;

    /*! Idem but output the max buffer. Dimension is tileXNum x tileYNum */
    void greyMaxRGBA(uint8 **rgba) const;

    const uint32 width;   //!< Width of the depth buffer  (multiple of Tile::width)
    const uint32 height;  //!< Height of the depth buffer (multiple of Tile::height)
    const uint32 pixelNum;//!< Total number of pixels
    const uint32 tileXNum;//!< Number of tiles per row
    const uint32 tileYNum;//!< Number of tiles per column
    const uint32 tileNum; //!< pixelNum / Tile::pixelNum
    Tile *tiles;          //!< The depth buffer data
    PF_STRUCT(HiZ);
  };

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
    PF_STRUCT(PerspectiveFrustum);
  };

// Do we want to use the HiZ buffer?
#define PF_HIZ_USE_ZBUFFER 1

// Growing the AABB allows us to use really small HiZ buffer with no visible
// artifacts. Should be a run-time value
#define PF_HIZ_GROW_AABB 1

  INLINE bool PerspectiveFrustum::isVisible(const RendererSegment &sgmt)
  {
#if PF_HIZ_GROW_AABB
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

#if PF_HIZ_USE_ZBUFFER
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
          PF_ASSERT(tileID < hiz->tileNum);
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
#endif /* PF_HIZ_USE_ZBUFFER */
  }

#undef PF_HIZ_USE_ZBUFFER
#undef PF_HIZ_GROW_AABB

} /* namespace pf */

#endif /* __PF_HIZ_HPP__ */


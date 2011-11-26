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
  struct HiZ : public RefCount
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
     *  colors. No sync is done, so the user is responsible for it
     */
    void greyRGBA(uint8 **pixels) const;

    const uint32 width;   //!< Width of the depth buffer  (multiple of Tile::width)
    const uint32 height;  //!< Height of the depth buffer (multiple of Tile::height)
    const uint32 pixelNum;//!< Total number of pixels
    const uint32 tileXNum;//!< Number of tiles per row
    const uint32 tileYNum;//!< Number of tiles per column
    const uint32 tileNum; //!< pixelNum / Tile::pixelNum
    Tile *tiles;          //!< The depth buffer data
  };

} /* namespace pf */

#endif /* __PF_HIZ_HPP__ */


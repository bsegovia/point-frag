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

#include "hiz.hpp"
#include "rt/intersector.hpp"
#include "rt/rt_camera.hpp"

namespace pf
{
  /*! Task responsible to fill the HiZ buffer */
  class TaskRayTraceHiZ : public TaskSet
  {
  public:
    /*! Set the fields manually */
    TaskRayTraceHiZ(size_t elemNum) : TaskSet(elemNum, "TaskRayTraceHiZ") {}

    /*! Ray trace a task tile (equal or bigger than a HiZ tile */
    virtual void run(size_t tileID);

    Ref<HiZ> zBuffer;                 //!< Keep the z buffer alive
    Ref<Intersector> intersector;     //!< Properly keep a reference on it
    RTCameraPacketGen gen;            //!< Generates the ray
    vec3f view;
    uint32 taskXNum;                  //!< Number of tasks along X axis
    uint32 taskYNum;                  //!< Number of tasks along Y axis
    static const uint32 width  = 16u; //!< Tile width processed per job ...
    static const uint32 height = 16u; //!< ... and its height
    static const uint32 pixelNum = width * height;
    STATIC_ASSERT(width % HiZ::Tile::width == 0);
    STATIC_ASSERT(height % HiZ::Tile::height == 0);
  };

  HiZ::HiZ(uint32 width_, uint32 height_) :
    width(ALIGN(width_, TaskRayTraceHiZ::width)),
    height(ALIGN(height_, TaskRayTraceHiZ::height)),
    pixelNum(width * height),
    tileXNum(width / Tile::width),
    tileYNum(height / Tile::height),
    tileNum(pixelNum / Tile::pixelNum)
  {
    const size_t tileSize = sizeof(Tile) * this->tileNum;
    this->tiles = (Tile*) PF_ALIGNED_MALLOC(tileSize, sizeof(ssef));
  }

  HiZ::~HiZ(void) { PF_ALIGNED_FREE(this->tiles); }

  // We create packets and directly fill each zBuffer tile. Note that we
  // really store t values
  void TaskRayTraceHiZ::run(size_t taskID)
  {
    const uint32 taskX = taskID % this->taskXNum;
    const uint32 taskY = taskID / this->taskXNum;
    const uint32 startX = taskX * this->width;
    const uint32 startY = taskY * this->height;
    const uint32 endX = startX + this->width;
    const uint32 endY = startY + this->height;
    uint32 tileY = startY / HiZ::Tile::height;

    for (uint32 y = startY; y < endY; y += RayPacket::height, ++tileY) {
      uint32 tileX = startX / HiZ::Tile::width;
      for (uint32 x = startX; x < endX; x += RayPacket::width, ++tileX) {
        RayPacket pckt;
        PacketHit hit;
        gen.generate(pckt, x, y);
        intersector->traverse(pckt, hit);
        ssef zmin(inf), zmax(neg_inf);
        const uint32 tileID = tileX + tileY * zBuffer->tileXNum;
        PF_ASSERT(tileID < zBuffer->tileNum);
        HiZ::Tile &tile = zBuffer->tiles[tileID];
        for (uint32 chunkID = 0; chunkID < HiZ::Tile::chunkNum; ++chunkID) {
          //const ssef t = hit.t[chunkID];
          const ssef t = hit.t[chunkID] *dot(sse3f(view.x,view.y,view.z), pckt.dir[chunkID]);
          tile.z[chunkID] = t;
          zmin = min(zmin, t);
          zmax = max(zmax, t);
        }
        tile.zmin = reduce_min(zmin)[0];
        tile.zmax = reduce_max(zmax)[0];
      }
    }
  }

  Ref<Task> HiZ::rayTrace(const RTCamera &cam, Ref<Intersector> intersector)
  {
    const size_t taskNum = this->pixelNum / TaskRayTraceHiZ::pixelNum;
    Ref<TaskRayTraceHiZ> task = PF_NEW(TaskRayTraceHiZ, taskNum);
    cam.createGenerator(task->gen, this->width, this->height);
    task->zBuffer = this;
    task->view = cam.view;
    task->intersector = intersector;
    task->taskXNum = this->width  / TaskRayTraceHiZ::width;
    task->taskYNum = this->height / TaskRayTraceHiZ::height;
    return task.cast<Task>();
  }

  void HiZ::greyRGBA(uint8 **pixels) const
  {
    if (UNLIKELY(pixels == NULL)) return;
    if (UNLIKELY(*pixels == NULL)) return;

    // Iterate over all pixels, find the tile, find the pixel in the tile and
    // output a grey scaled pixel
    uint8 *rgba = *pixels;
    for (uint32 y = 0; y < this->height; ++y)
    for (uint32 x = 0; x < this->width; ++x) {
      const uint32 tileX = x / Tile::width;
      const uint32 tileY = y / Tile::height;
      const uint32 tileID = tileX + tileY * this->tileXNum;
      const Tile &tile = this->tiles[tileID];
      const float *zptr = &tile.z[0][0];
      const uint32 offsetX = x % Tile::width;
      const uint32 offsetY = y % Tile::height;
      const uint32 offset = offsetX + Tile::width * offsetY;
      const uint32 imageOffset = (x + y * this->width) * 4;
      const float z = min(zptr[offset] * 32.f, 255.f);
      PF_ASSERT(imageOffset < 4*this->pixelNum);
      rgba[imageOffset + 0] = uint8(z);
      rgba[imageOffset + 1] = uint8(z);
      rgba[imageOffset + 2] = uint8(z);
      rgba[imageOffset + 3] = 0xff;
    }
  }

  template <bool outputMin>
  INLINE void HiZGreyMinMax(const HiZ &hiz, uint8 **pixels)
  {
    if (UNLIKELY(pixels == NULL)) return;
    if (UNLIKELY(*pixels == NULL)) return;

    // Iterate over all pixels, find the tile, find the pixel in the tile and
    // output a grey scaled pixel
    uint8 *rgba = *pixels;
    for (uint32 y = 0; y < hiz.tileYNum; ++y)
    for (uint32 x = 0; x < hiz.tileXNum; ++x) {
      const uint32 tileID = x + y * hiz.tileXNum;
      const HiZ::Tile &tile = hiz.tiles[tileID];
      const float z = min((outputMin ? tile.zmin : tile.zmax) * 64.f, 255.f);
      rgba[4*tileID + 0] = uint8(z);
      rgba[4*tileID + 1] = uint8(z);
      rgba[4*tileID + 2] = uint8(z);
      rgba[4*tileID + 3] = 0xff;
    }
  }

  void HiZ::greyMinRGBA(uint8 **pixels) const { HiZGreyMinMax<true>(*this, pixels); }
  void HiZ::greyMaxRGBA(uint8 **pixels) const { HiZGreyMinMax<false>(*this, pixels); }

} /* namespace pf */


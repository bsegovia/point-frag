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
  HiZ::HiZ(uint32 width_, uint32 height_) :
    width(ALIGN(width_, Tile::width)),
    height(ALIGN(height_, Tile::height)),
    pixelNum(width * height),
    tileXNum(width / Tile::width),
    tileYNum(height / Tile::height),
    tileNum(pixelNum / Tile::pixelNum)
  {
    const size_t tileSize = sizeof(Tile) * this->tileNum;
    this->tiles = (Tile*) PF_ALIGNED_MALLOC(tileSize, sizeof(ssef));
  }

  HiZ::~HiZ(void) { PF_ALIGNED_FREE(this->tiles); }

  /*! Task responsible to fill the HiZ buffer */
  class TaskRayTraceHiZ : public TaskSet
  {
  public:
    /*! Set the fields manually */
    TaskRayTraceHiZ(size_t elemNum) : TaskSet(elemNum, "TaskRayTraceHiZ") {}

    /*! Ray trace a task tile (equal or bigger than a HiZ tile */
    virtual void run(size_t tileID);

    Ref<HiZ> zBuffer;                //!< Keep the z buffer alive
    Ref<Intersector> intersector;    //!< Properly keep a reference on it
    RTCameraPacketGen gen;           //!< Generates the ray
    static const uint32 width  = 32; //!< Tile width processed per job ...
    static const uint32 height = 32; //!< ... and its height
    static const uint32 pixelNum = width * height;
  };

  // We create packets and directly fill each zBuffer tile
  void TaskRayTraceHiZ::run(size_t taskID)
  {
    const uint32 offset = taskID * this->pixelNum;
    const uint32 startX = offset % zBuffer->width;
    const uint32 startY = offset / zBuffer->width;
    const uint32 endX = startX + this->width;
    const uint32 endY = startY + this->height;
    uint32 tileX = startX / HiZ::Tile::width;
    uint32 tileY = startY / HiZ::Tile::height;

    for (uint32 y = startY; y < endY; y += RayPacket::height, ++tileY)
    for (uint32 x = startX; x < endX; x += RayPacket::width, ++tileX) {
      RayPacket packet;
      PacketHit hit;
      gen.generate(packet, x, y);
      intersector->traverse(packet, hit);
    }
  }

  Ref<Task> HiZ::rayTrace(const RTCamera &cam, Intersector &intersector)
  {
    const size_t taskNum = this->pixelNum / TaskRayTraceHiZ::pixelNum;
    Ref<TaskRayTraceHiZ> task = PF_NEW(TaskRayTraceHiZ, taskNum);
    cam.createGenerator(task->gen, this->width, this->height);
    task->zBuffer = this;
    task->intersector = &intersector;
    task->scheduled();
    return task.cast<Task>();
  }

} /* namespace pf */


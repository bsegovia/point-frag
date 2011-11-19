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

#ifndef __PF_RAY_PACKET_HPP__
#define __PF_RAY_PACKET_HPP__

#include "simd/ssef.hpp"
#include "simd/sse_vec.hpp"
#include "sys/platform.hpp"

namespace pf
{
  // Bitfield to figure out what the packet supports
  static const uint32 RAY_PACKET_IA = 1 << 0; //!< Does it have interval arithmetic?
  static const uint32 RAY_PACKET_CR = 1 << 1; //!< Does it have corner rays?
  static const uint32 RAY_PACKET_CO = 1 << 2; //!< Do rays have common origin?

  /*! Packet of rays grouped together for faster traversal */
  struct RayPacket
  {
    static const uint32 laneNum = sizeof(ssef) / sizeof(int32);
    static const uint32 width  = 16;
    static const uint32 height = 16;
    static const uint32 rayNum = width * height;
    static const uint32 packetNum = rayNum / laneNum;
    static const ssef crx;//!< X coordinates of the 4 corner rays
    static const ssef cry;//!< Y coordinates of the 4 corner rays
    sse3f org[packetNum]; //!< Origin of each ray
    sse3f dir[packetNum]; //!< Ray direction (not required to be normalized)
    sse3f rdir[packetNum];//!< Rcp of directions for box intersection
    sse3f crdir;          //!< Direction of corner rays (only primary rays)
    ssef iaMinOrg;        //!< Minimum origin (for interval arithmetic)
    ssef iaMaxOrg;        //!< Maximum origin. MIN == MAX if common origin
    ssef iaMinrDir;       //!< Minimum rcp(direction) for IA
    ssef iaMaxrDir;       //!< Maximum rcp(direction) for IA
    sseb iasign;          //!< Sign of the IA directions
    uint32 signs;         //!< Says which directions have different signs
    uint32 cr;            //!< Does the packet have corner rays?
    uint32 co;            //!< Does the packet have a common origin?
    uint32 ia;            //!< Does the packet use interval arithmetic?
  };

  /*! Set of hit points for a ray packet */
  struct PacketHit
  {
    INLINE PacketHit(void) {
      for (size_t i = 0; i < RayPacket::packetNum; ++i) t[i] = FLT_MAX;
      for (size_t i = 0; i < RayPacket::packetNum; ++i) id0[i] = -1;
    }
    ssef t[RayPacket::packetNum];   //!< Distance intersection
    ssef u[RayPacket::packetNum];   //!< u barycentric coordinate
    ssef v[RayPacket::packetNum];   //!< v barycentric coordinate
    ssei id0[RayPacket::packetNum]; //!< First ID of intersection
    ssei id1[RayPacket::packetNum]; //!< Second ID of intersection
  };

} /* namespace pf */

#endif /* __PF_RAY_PACKET_HPP__ */


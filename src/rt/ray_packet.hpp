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
    static const uint32 width  = 8;
    static const uint32 height = 8;
    static const uint32 rayNum = width * height;
    static const uint32 chunkNum = rayNum / laneNum;
    static const ssef crx;//!< X coordinates of the 4 corner rays
    static const ssef cry;//!< Y coordinates of the 4 corner rays
    sse3f org[chunkNum]; //!< Origin of each ray
    sse3f dir[chunkNum]; //!< Ray direction (not required to be normalized)
    sse3f rdir[chunkNum];//!< Rcp of directions for box intersection
    sse3f crdir;         //!< Direction of corner rays (only primary rays)
    ssef iaMinOrg;       //!< Minimum origin (for interval arithmetic)
    ssef iaMaxOrg;       //!< Maximum origin. MIN == MAX if common origin
    ssef iaMinrDir;      //!< Minimum rcp(direction) for IA
    ssef iaMaxrDir;      //!< Maximum rcp(direction) for IA
    sseb iasign;         //!< Sign of the ray directions
    uint32 properties;   //!< CR / IA / CO (see above)
    PF_STRUCT(RayPacket);
  };

  /*! Set of hit points for a ray packet */
  struct PacketHit
  {
    INLINE PacketHit(void) {
      for (uint32 i = 0; i < RayPacket::chunkNum; ++i) t[i] = FLT_MAX;
      for (uint32 i = 0; i < RayPacket::chunkNum; ++i) id0[i] = -1;
    }
    ssef t[RayPacket::chunkNum];   //!< Distance intersection
    ssef u[RayPacket::chunkNum];   //!< u barycentric coordinate
    ssef v[RayPacket::chunkNum];   //!< v barycentric coordinate
    ssei id0[RayPacket::chunkNum]; //!< First ID of intersection
    ssei id1[RayPacket::chunkNum]; //!< Second ID of intersection
    PF_STRUCT(PacketHit);
  };

} /* namespace pf */

#endif /* __PF_RAY_PACKET_HPP__ */

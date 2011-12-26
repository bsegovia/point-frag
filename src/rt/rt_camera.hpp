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

#ifndef __PF_RT_CAMERA_HPP__
#define __PF_RT_CAMERA_HPP__

#include "rt/ray.hpp"
#include "rt/ray_packet.hpp"
#include "simd/ssef.hpp"
#include "simd/ssei.hpp"
#include "math/vec.hpp"
#include "math/matrix.hpp"

namespace pf
{
  struct RTCameraRayGen;    //!< Outputs (single) rays
  struct RTCameraPacketGen; //!< Outputs packets of rays

  static const float znear = 0.1f;   // XXX use CVAR
  static const float zfar = 10000.f; // XXX use CVAR

  /*! Standard pinhole camera */
  class RTCamera
  {
  public:
    /*! Build a perspective camera */
    RTCamera(const vec3f &org,
             const vec3f &up,
             const vec3f &view,
             float fov,
             float ratio);
    /*! Build the ray generator */
    void createGenerator(RTCameraRayGen &gen, int w, int h) const;
    /*! Build the ray packet generator */
    void createGenerator(RTCameraPacketGen &gen, int w, int h) const;
    /*! Build the 4x4 projection matrix from the camera */
    INLINE mat4x4f getMatrix(void) const {
      const mat4x4f P = pf::perspective(fov, ratio, znear, zfar);
      const mat4x4f V = pf::lookAt(org, org + view, up);
      return P*V;
    }
    ALIGNED(16) vec3f org;            //!< Origin of the camera
    ALIGNED(16) vec3f up;             //!< Up vector
    ALIGNED(16) vec3f view;           //!< View direction
    ALIGNED(16) vec3f imagePlaneOrg;  //!< Origin of the image plane
    ALIGNED(16) vec3f xAxis;          //!< X axis of the image plane
    ALIGNED(16) vec3f zAxis;          //!< Z axis of the image plane
    float fov, ratio, dist;
    PF_CLASS(RTCamera);
  };

  /*! Generate single rays from a pinhole camera */
  struct RTCameraRayGen
  {
    /*! Generate a ray at pixel (x,y) */
    INLINE void generate(Ray &ray, int x, int y) const;
    ALIGNED(16) vec3f org;            //!< Origin of the camera
    ALIGNED(16) vec3f imagePlaneOrg;  //!< Position of the image plane origin
    ALIGNED(16) vec3f xAxis;          //!< X axis of the image plane
    ALIGNED(16) vec3f zAxis;          //!< Y axis of the image plane
    PF_CLASS(RTCameraRayGen);
  };

  /*! Generate packet of rays from a pinhole camera */
  struct RTCameraPacketGen
  {
    /*! Generate a ray at pixel (x,y) */
    INLINE void generate(RayPacket &pckt, int x, int y) const;
    /*! Idem but with a z-curve order for the rays */
    INLINE void generateMorton(RayPacket &pckt, int x, int y) const;
    /*! Look up table for Morton curve (X coordinate) */
    static const int32 mortonX[];
    /*! Look up table for Morton curve (Y coordinate) */
    static const int32 mortonY[];
  private:
    friend class RTCamera; //!< Create this structure
    /*! Generate the rays */
    INLINE void generateRay(RayPacket &pckt, int x, int y) const;
    /*! Generate the ray data (in Morton order) */
    INLINE void generateRayMorton(RayPacket &pckt, int x, int y) const;
    /*! Generate the corner rays */
    INLINE void generateCR(RayPacket &pckt, int x, int y) const;
    /*! Generate the interval arithmetic vector. Says if IA can be used */
    INLINE bool generateIA(RayPacket &pckt, int x, int y) const;
    sse3f org;           //!< Origin
    sse3f imagePlaneOrg; //!< Image plane origin
    sse3f xAxis;         //!< X axis of the image plane
    sse3f zAxis;         //!< Z axis of the image plane
    ssef aOrg;           //!< Origin (AoS format)
    ssef aImagePlaneOrg; //!< Image plane origin (AoS format)
    ssef axAxis;         //!< X axis of the image plane (AoS format)
    ssef azAxis;         //!< Z axis of the image plane (AoS format)
    PF_CLASS(RTCameraPacketGen);
  };

  ///////////////////////////////////////////////////////////////////////////
  /// Implementation of the inlined functions
  ///////////////////////////////////////////////////////////////////////////

  INLINE void RTCameraRayGen::generate(Ray &ray, int x, int y) const
  {
    ray.org = org;
    ray.dir = imagePlaneOrg + float(x) * xAxis + float(y) * zAxis;
    ray.dir = normalize(ray.dir);
    ray.rdir = rcp(ray.dir);
    ray.tnear = 0.f;
    ray.tfar = FLT_MAX;
  }

  INLINE void RTCameraPacketGen::generateRay(RayPacket &pckt, int x, int y) const
  {
    const sse3f org(aOrg.xxxx(), aOrg.yyyy(), aOrg.zzzz());
    const ssef left = ssef(ssei(x))+ ssef::identity();
    ssef row = ssef(ssei(y));
    uint32 id = 0;

    // avoid issues with unused w
    pckt.iaMinOrg = pckt.iaMaxOrg = aOrg.xyzz();
    for(uint32 j = 0; j < pckt.height; ++j) {
      ssef column = left;
      for(uint32 i = 0; i < pckt.width; i += ssef::laneNum(), ++id) {
        pckt.org[id]   = org;
        pckt.dir[id].x = fixup(imagePlaneOrg.x + column*xAxis.x + row*zAxis.x);
        pckt.dir[id].y = fixup(imagePlaneOrg.y + column*xAxis.y + row*zAxis.y);
        pckt.dir[id].z = fixup(imagePlaneOrg.z + column*xAxis.z + row*zAxis.z);
        pckt.dir[id].x = (imagePlaneOrg.x + column*xAxis.x + row*zAxis.x);
        pckt.dir[id].y = (imagePlaneOrg.y + column*xAxis.y + row*zAxis.y);
        pckt.dir[id].z = (imagePlaneOrg.z + column*xAxis.z + row*zAxis.z);
        pckt.dir[id] = normalize(pckt.dir[id]);
        pckt.rdir[id].x = rcp(pckt.dir[id].x);
        pckt.rdir[id].y = rcp(pckt.dir[id].y);
        pckt.rdir[id].z = rcp(pckt.dir[id].z);
        column += ssef::laneNumv();
      }
      row += ssef::one();
    }
  }

  INLINE void RTCameraPacketGen::generateRayMorton(RayPacket &pckt, int x, int y) const
  {
    const sse3f org(aOrg.xxxx(), aOrg.yyyy(), aOrg.zzzz());
    const ssef left = ssef(ssei(x));
    const ssef top  = ssef(ssei(y));
    uint32 id = 0;

    // avoid issues with unused w
    pckt.iaMinOrg = pckt.iaMaxOrg = aOrg.xyzz();
    for(uint32 j = 0; j < pckt.height; ++j) {
      for(uint32 i = 0; i < pckt.width; i += ssef::laneNum(), ++id) {
        const ssei xcoord = ssei(mortonX + 4 * id);
        const ssei ycoord = ssei(mortonY + 4 * id);
        const ssef column  = left + ssef(xcoord);
        const ssef row = top  + ssef(ycoord);
        pckt.org[id]   = org;
        pckt.dir[id].x = fixup(imagePlaneOrg.x + column*xAxis.x + row*zAxis.x);
        pckt.dir[id].y = fixup(imagePlaneOrg.y + column*xAxis.y + row*zAxis.y);
        pckt.dir[id].z = fixup(imagePlaneOrg.z + column*xAxis.z + row*zAxis.z);
        pckt.dir[id] = normalize(pckt.dir[id]);
        pckt.rdir[id].x = rcp(pckt.dir[id].x);
        pckt.rdir[id].y = rcp(pckt.dir[id].y);
        pckt.rdir[id].z = rcp(pckt.dir[id].z);
      }
    }
  }

  // Unused since we do not use back face culling right now
  INLINE void RTCameraPacketGen::generateCR(RayPacket &pckt, int x, int y) const
  {
    const ssef left = ssef(ssei(x)) + pckt.crx;
    const ssef top  = ssef(ssei(y)) + pckt.cry;
    pckt.crdir.x = imagePlaneOrg.x + left*xAxis.x + top*zAxis.x;
    pckt.crdir.y = imagePlaneOrg.y + left*xAxis.y + top*zAxis.y;
    pckt.crdir.z = imagePlaneOrg.z + left*xAxis.z + top*zAxis.z;
  }

  INLINE bool RTCameraPacketGen::generateIA(RayPacket &pckt, int x, int y) const
  {
    const ssef fw = (float) pckt.width;
    const ssef fh = (float) pckt.height;
    const ssef fx = (float) x;
    const ssef fy = (float) y;
    const ssef bottomLeft = aImagePlaneOrg + fx * axAxis + fy * azAxis;
    const ssef bottomRight = bottomLeft + fw * axAxis;
    const ssef topRight = bottomRight + fh * azAxis;
    const ssef topLeft = bottomLeft + fh * azAxis;
    const ssef dmin = fixup(min(min(bottomLeft, bottomRight), min(topLeft, topRight)));
    const ssef dmax = fixup(max(max(bottomLeft, bottomRight), max(topLeft, topRight)));
    const ssef rcpMin = rcp(dmax).xyzz(); // avoid issues with unused w
    const ssef rcpMax = rcp(dmin).xyzz(); // avoid issues with unused w
    const ssef minusMin = -rcpMin;
    const ssef minusMax = -rcpMax;
    const size_t mask = movemask(rcpMin);
    const sseb maskv = unmovemask(mask);
    pckt.iasign = maskv;
    pckt.iaMinrDir = select(maskv, minusMax, rcpMin);
    pckt.iaMaxrDir = select(maskv, minusMin, rcpMax);
    return movemask(dmin ^ dmax) == 0;
  }

  INLINE void RTCameraPacketGen::generate(RayPacket &pckt, int x, int y) const
  {
    this->generateRay(pckt,x,y);
    // Unused since we do not use back face culling right now
    this->generateCR(pckt,x,y);
    pckt.properties = RAY_PACKET_CO;
    if (this->generateIA(pckt,x,y))
      pckt.properties |= RAY_PACKET_IA;
  }

  INLINE void RTCameraPacketGen::generateMorton(RayPacket &pckt, int x, int y) const
  {
    this->generateRayMorton(pckt,x,y);
    // Unused since we do not use back face culling right now
    // this->generateCR(pckt,x,y);
    pckt.properties = RAY_PACKET_CO;
    if (this->generateIA(pckt,x,y))
      pckt.properties |= RAY_PACKET_IA;
  }

} /* namespace pf */

#endif /* __PF_RT_CAMERA_HPP__ */


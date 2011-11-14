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

#include "ray.hpp"
#include "math/vec.hpp"

namespace pf
{
  struct RTCameraRayGenerator;

  /*! Standard pinhole camera */
  class RTCamera
  {
  public:
    /*! Build a perspective camera */
    RTCamera(const vec3f &org,
             const vec3f &to,
             const vec3f &up,
             float angle,
             float aspect);
    /*! Build the ray generator */
    void createGenerator(RTCameraRayGenerator &gen, int w, int h) const;
  private:
    ALIGNED(16) vec3f org;            //!< Origin of the camera
    ALIGNED(16) vec3f dir;            //!< View direction
    ALIGNED(16) vec3f up;             //!< Up vector
    ALIGNED(16) vec3f xAxis;          //!< First vector of the basis
    ALIGNED(16) vec3f zAxis;          //!< Second vector
    ALIGNED(16) vec3f imagePlaneOrg;  //!< Origin of the image plane
    float angle, aspect, dist;
  };

  /*! Generate rays from a pinhole camera */
  struct RTCameraRayGenerator
  {
    /*! Generate a ray at pixel (x,y) */
    INLINE Ray generate(int x, int y) {
      Ray ray;
      ray.org = org;
      ray.dir = imagePlaneOrg + float(x) * xAxis + float(y) * zAxis;
      ray.dir = normalize(ray.dir);
      ray.rdir = rcp(ray.dir);
      ray.near = 0.f;
      ray.far = FLT_MAX;
      return ray;
    }
    ALIGNED(16) vec3f org;
    ALIGNED(16) vec3f xAxis;
    ALIGNED(16) vec3f zAxis;
    ALIGNED(16) vec3f imagePlaneOrg;
  };

} /* namespace pf */

#endif /* __PF_RT_CAMERA_HPP__ */


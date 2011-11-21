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

#include "rt_camera.hpp"

namespace pf
{
  RTCamera::RTCamera(const vec3f &org_,
                     const vec3f &to_,
                     const vec3f &up_,
                     float angle_,
                     float aspect_)
  {
    this->org = org_;
    this->dir = to_ - org_;
    this->up = up_;
    this->angle = angle_;
    this->aspect = aspect_;
    this->dist = 0.5f / tan((float)(this->angle * (float) M_PI / 360.f));
    this->dir = normalize(this->dir);
    this->up = normalize(this->up);

    const float left = -this->aspect * 0.5f;
    const float top = 0.5f;
    const vec3f &yAxis = this->dir;
    this->xAxis = cross(yAxis, this->up);
    this->xAxis = normalize(this->xAxis);
    this->zAxis = cross(yAxis, this->xAxis);
    this->zAxis = normalize(this->zAxis);
    this->imagePlaneOrg = this->dist * yAxis + left * this->xAxis - top * this->zAxis;
    this->xAxis *= this->aspect;
  }

  void RTCamera::createGenerator(RTCameraRayGen &gen, int w, int h) const
  {
    const float rw = 1.f / (float) w;
    const float rh = 1.f / (float) h;
    gen.org.x = this->org.x;
    gen.org.y = this->org.y;
    gen.org.z = this->org.z;
    gen.imagePlaneOrg.x = this->imagePlaneOrg.x;
    gen.imagePlaneOrg.y = this->imagePlaneOrg.y;
    gen.imagePlaneOrg.z = this->imagePlaneOrg.z;
    gen.xAxis.x = this->xAxis.x * rw;
    gen.xAxis.y = this->xAxis.y * rw;
    gen.xAxis.z = this->xAxis.z * rw;
    gen.zAxis.x = this->zAxis.x * rh;
    gen.zAxis.y = this->zAxis.y * rh;
    gen.zAxis.z = this->zAxis.z * rh;
  }

  void RTCamera::createGenerator(RTCameraPacketGen &gen, int w, int h) const
  {
    const float rw = 1.f / (float) w;
    const float rh = 1.f / (float) h;
    gen.aOrg = ssef(&this->org.x);
    gen.aImagePlaneOrg = ssef(&this->imagePlaneOrg.x);
    gen.axAxis = ssef(&this->xAxis.x) * rw;
    gen.azAxis = ssef(&this->zAxis.x) * rh;
    gen.org    = sse3f(gen.aOrg.xxxx(),   gen.aOrg.yyyy(),   gen.aOrg.zzzz());
    gen.xAxis  = sse3f(gen.axAxis.xxxx(), gen.axAxis.yyyy(), gen.axAxis.zzzz());
    gen.zAxis  = sse3f(gen.azAxis.xxxx(), gen.azAxis.yyyy(), gen.azAxis.zzzz());
    gen.imagePlaneOrg = sse3f(gen.aImagePlaneOrg.xxxx(),
                              gen.aImagePlaneOrg.yyyy(),
                              gen.aImagePlaneOrg.zzzz());
  }

// Look-up tables for z ordered rays
#include "morton.hxx"

} /* namespace pf */


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
#include <cstdlib>

namespace pf
{
  RTCamera::RTCamera(const vec3f &org_,
                     const vec3f &up_,
                     const vec3f &view_,
                     float fov_,
                     float ratio_)
  {
    this->org = org_;
    this->up = up_;
    this->view = view_;
    this->fov = fov_;
    this->ratio = ratio_;
    this->dist = 0.5f / tan((float)(this->fov * float(pi) / 360.f));
    this->view = normalize(this->view);
    this->up = normalize(this->up);

    const float left = -this->ratio * 0.5f;
    const float top = 0.5f;
    const vec3f &yAxis = this->view;
    this->xAxis = cross(yAxis, this->up);
    this->xAxis = normalize(this->xAxis);
    this->zAxis = cross(yAxis, this->xAxis);
    this->zAxis = normalize(this->zAxis);
    this->imagePlaneOrg = this->dist * yAxis + left * this->xAxis - top * this->zAxis;
    this->xAxis *= this->ratio;
  }

  void RTCamera::createGenerator(RTCameraRayGen &gen, int w, int h) const
  {
    const float rw = 1.f / (float) w;
    const float rh = 1.f / (float) h;
    gen.org = this->org;
    gen.imagePlaneOrg = this->imagePlaneOrg;
    gen.xAxis = this->xAxis * rw;
    gen.zAxis = this->zAxis * rh;
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


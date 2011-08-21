// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
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

#ifndef __PF_RAY_H__
#define __PF_RAY_H__

#include "common/default.hpp"

namespace pf
{
  /*! Ray structure. Contains all information about a ray including
   *  precomputed reciprocal direction. */
  struct Ray
  {
    INLINE Ray() {}
    INLINE Ray(vec3f org, vec3f dir, float near = zero, float far = inf)
      : org(org), dir(dir), rdir(1.0f/dir), near(near), far(far) {}

  public:
    vec3f org;     //!< Ray origin
    vec3f dir;     //!< Ray direction
    vec3f rdir;    //!< Reciprocal ray direction
    float near;    //!< Start of ray segment
    float far;     //!< End of ray segment
  };

  /*! Outputs ray to stream. */
  inline std::ostream& operator<<(std::ostream& cout, const Ray& ray) {
    return cout << "{ org = " << ray.org << ", dir = " << ray.dir << ", near = " << ray.near << ", far = " << ray.far << " }";
  }
}

#endif

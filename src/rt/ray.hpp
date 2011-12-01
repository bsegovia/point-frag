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

#ifndef __PF_RAY_HPP__
#define __PF_RAY_HPP__

#include "math/vec.hpp"

namespace pf
{
  /*! Single ray */
  struct Ray
  {
    /*! Default construction does nothing. */
    INLINE Ray(void) {}
    /*! Ray from origin, direction, and ray segment. near < far */
    INLINE Ray(const vec3f& org, const vec3f& dir, float near = zero, float far = inf)
      : org(org), dir(dir), rdir(1.0f/dir), tnear(near), tfar(far) {}
    ALIGNED(16) vec3f org;  //!< Origin of the ray
    ALIGNED(16) vec3f dir;  //!< Direction of it
    ALIGNED(16) vec3f rdir; //!< Per-compoment inverse of the direction
    float tnear, tfar;      //!< Valid segment along the direction
  };

  /*! Hit point */
  struct Hit
  {
    /*! an ID == -1 means no intersection */
    INLINE Hit(void) : t(FLT_MAX), id0(-1), id1(-1) {}
    /*! Tests if we hit something. */
    INLINE operator bool(void) { return id0 != -1; }
    ALIGNED(16) float t, u, v, w; //!< Intersection coordinates (in the triangle)
    int id0, id1;                 //!< Two IDs hit
  };

} /* namespace pf */

#endif /* __PF_RAY_HPP__ */


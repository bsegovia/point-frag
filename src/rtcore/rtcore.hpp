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

#ifndef __PF_RTCORE_H__
#define __PF_RTCORE_H__

#include "ray.hpp"
#include "hit.hpp"

namespace pf
{
  /*! Single ray interface to the traverser. A closest intersection
   *  point of a ray with the geometry can be found. A ray can also be
   *  tested for occlusion by any geometry. */
  class Intersector : public RefCount {
  public:

    /*! A virtual destructor is required. */
    virtual ~Intersector() {}

    /*! Intersects the ray with the geometry and returns the hit
     *  information. */
    virtual void intersect(const Ray& ray, Hit& hit) const = 0;

    /*! Tests the ray for occlusion with the scene. */
    virtual bool occluded(const Ray& ray) const = 0;
  };

  /*! Triangle interface structure to the builder. The builders get an
   *  array of triangles represented by this structure as input. */
  struct ALIGNED(16) BuildTriangle
  {
    /*! Constructs a builder triangle. */
    INLINE BuildTriangle(const vec3f& v0,   //!< 1st vertex of the triangle
                         const vec3f& v1,   //!< 2nd vertex of the triangle
                         const vec3f& v2,   //!< 3rd vertex of the triangle
                         int id0 = 0,       //!< 1st optional user ID
                         int id1 = 0,       //!< 2nd optional user ID
                         int id2 = 0)       //!< 3rd optional user ID
      : x0(v0.x), y0(v0.y), z0(v0.z), id0(id0),
        x1(v1.x), y1(v1.y), z1(v1.z), id1(id1),
        x2(v2.x), y2(v2.y), z2(v2.z), id2(id2) { }

    INLINE ssef& v0() const { return *(ssef*)&x0; }
    INLINE ssef& v1() const { return *(ssef*)&x1; }
    INLINE ssef& v2() const { return *(ssef*)&x2; }
    INLINE const ssef& operator[](size_t idx) const { return ((ssef*)this)[idx]; }

  public:
    float x0,y0,z0; int id0;    //!< 1st triangle vertex and ID
    float x1,y1,z1; int id1;    //!< 2nd triangle vertex and ID
    float x2,y2,z2; int id2;    //!< 3rd triangle vertex and ID
  };

  /*! Creates acceleration structure of specified type. */
  Intersector* rtcCreateAccel(const char* type, const BuildTriangle* triangles, size_t numTriangles);
}

#endif

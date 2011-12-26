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

#ifndef __OBJ_HPP__
#define __OBJ_HPP__

#include "sys/platform.hpp"
#include "sys/filename.hpp"
#include "math/vec.hpp"

namespace pf
{

  /*! OBJ - an obj file (vertices are processed and merged) */
  struct Obj : public NonCopyable
  {
    /*! OBJ triangle - indexes vertices and material */
    struct Triangle {
      INLINE Triangle(void) {}
      INLINE Triangle(vec3i v_, int m_) : v(v_), m(m_) {}
      vec3i v;
      int m;
      PF_STRUCT(Triangle);
    };

    /*! OBJ vertex - stores position, normal and texture coordinates */
    struct Vertex {
      INLINE Vertex(void) {}
      INLINE Vertex(vec3f p_, vec3f n_, vec2f t_) :
        p(p_), n(n_), t(t_) {}
      vec3f p, n;
      vec2f t;
      PF_STRUCT(Vertex);
    };

    /*! OBJ material group - triangles are grouped by material */
    struct MatGroup {
      MatGroup(int first_, int last_, int m_) :
        first(first_), last(last_), m(m_) {}
      MatGroup(void) {}
      int first, last, m;
      PF_STRUCT(MatGroup);
    };

    /*! OBJ Material - just a dump of mtl description */
    struct Material {
      char *name;
      char *map_Ka;
      char *map_Kd;
      char *map_D;
      char *map_Bump;
      double amb[3];
      double diff[3];
      double spec[3];
      double km;
      double reflect;
      double refract;
      double trans;
      double shiny;
      double glossy;
      double refract_index;
      PF_STRUCT(Material);
    };

    Obj(void);
    ~Obj(void);
    INLINE bool isValid(void) const {return this->triNum > 0;}
    bool load(const FileName &fileName);
    Triangle *tri;
    Vertex *vert;
    MatGroup *grp;
    Material *mat;
    size_t triNum;
    size_t vertNum;
    size_t grpNum;
    size_t matNum;
    PF_STRUCT(Obj);
  };
} /* namespace pf */

#endif /* __OBJ_HPP__ */


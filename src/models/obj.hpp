#ifndef __OBJ_HPP__
#define __OBJ_HPP__

#include "sys/platform.hpp"
#include "math/vec.hpp"

namespace pf
{
  // OBJ triangle - indexes vertices and material
  struct ObjTriangle {
    INLINE ObjTriangle(void) {}
    INLINE ObjTriangle(vec3i v_, int m_) : v(v_), m(m_) {}
    vec3i v;
    int m;
  };

  // OBJ vertex - stores position, normal and texture coordinates
  struct ObjVertex {
    INLINE ObjVertex(void) {}
    INLINE ObjVertex(vec3f p_, vec3f n_, vec2f t_) :
      p(p_), n(n_), t(t_) {}
    vec3f p, n;
    vec2f t;
  };

  // OBJ material group - triangles are grouped by material
  struct ObjMatGroup { int first, last, m; };

  // OBJ Material - just a dump of mtl description
  struct ObjMaterial {
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
  };

  // OBJ - an obj file (vertices are processed and merged)
  struct Obj {
    Obj(void);
    ~Obj(void);
    bool load(const char *fileName);
    ObjTriangle *tri;
    ObjVertex *vert;
    ObjMatGroup *grp;
    ObjMaterial *mat;
    size_t triNum;
    size_t vertNum;
    size_t grpNum;
    size_t matNum;
  };
}

#endif /* __OBJ_HPP__ */


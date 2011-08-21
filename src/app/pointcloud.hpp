#ifndef __POINTCLOUD_HPP__
#define __POINTCLOUD_HPP__

#include "math/vec.hpp"

namespace pf {
  /*! OBJ mesh */
  struct Obj;

  /*! A point is position, normal and color */
  struct Point {
    INLINE Point() {}
    INLINE Point(ZeroTy) : p(0.f), child(0) {
      for (int i=0; i<4; ++i) n[i]=t[i]=c[i]=0;
    }

    /*! Quantize / unquantize normal and tangent */
    #define DECL_ENCODE_DECODE(FIELD, NAME, DEC_SCALE, ENC_SCALE) \
      INLINE vec3f get##NAME(void) const {                        \
        vec3f to;                                                 \
        to.x = float(this->FIELD[0]) / 255.f * DEC_SCALE;         \
        to.y = float(this->FIELD[1]) / 255.f * DEC_SCALE;         \
        to.z = float(this->FIELD[2]) / 255.f * DEC_SCALE;         \
        return to;                                                \
      }                                                           \
      INLINE void set##NAME(vec3f v) {                            \
        this->FIELD[0] = 255.f * (v.x * ENC_SCALE);               \
        this->FIELD[1] = 255.f * (v.y * ENC_SCALE);               \
        this->FIELD[2] = 255.f * (v.z * ENC_SCALE);               \
      }
    DECL_ENCODE_DECODE(n, Normal, 2.f - 1.f, 0.5f + 0.5f)
    DECL_ENCODE_DECODE(t, Tangent, 2.f - 1.f, 0.5f + 0.5f)
    DECL_ENCODE_DECODE(c, Color, 1.f, 1.f)
    #undef DECL_ENCODE_DECODE

    vec3f p;
    uint8_t n[4], t[4], c[4];
    uint32_t child;
  };

  /*! Sample points along the surface of an OBJ mesh */
  Point* buildPointCloud(const Obj&, size_t &pointNum, float density); // pt.m^2
}

#endif /* __POINTCLOUD_HPP__ */


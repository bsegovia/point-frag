#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include "math/vec.hpp"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace pf
{
  INLINE double equi(uint32_t i, uint32_t n, uint32_t r) {
    r ^= i * (0x100000000LL / n);
    return static_cast<double>(r) / static_cast<double>(0x100000000LL);
  }

  INLINE double sobol(uint32_t i, uint32_t r) { 
    for(uint32_t v = 1U<<31; i; i >>= 1, v ^= v>>1)
      if(i & 1) r ^= v;
    return static_cast<double>(r) / static_cast<double>(0x100000000LL);
  }

  INLINE double corput(uint32_t i, uint32_t r = 0u) { 
    i = ( i << 16) | ( i >> 16);
    i = ((i & 0x00ff00ff) << 8) | ((i & 0xff00ff00) >> 8);
    i = ((i & 0x0f0f0f0f) << 4) | ((i & 0xf0f0f0f0) >> 4);
    i = ((i & 0x33333333) << 2) | ((i & 0xcccccccc) >> 2);
    i = ((i & 0x55555555) << 1) | ((i & 0xaaaaaaaa) >> 1);
    i ^= r; 
    return static_cast<double>(i) / static_cast<double>(0x100000000LL);
  }

  INLINE double larcher(uint32_t i, uint32_t r) {
    for(uint32_t v = 1U<<31; i; i >>= 1, v |= v>>1)
      if(i & 1) r ^= v; 
    return static_cast<double>(r) / static_cast<double>(0x100000000LL);
  }

  INLINE vec2f larcher2D(uint32_t i, uint32_t n, uint32_t sx = 0u, uint32_t sy = 0u) {
    return vec2f(equi(i, n, sx), larcher(i, sy));
  }

  INLINE vec2f sobol2D(uint32_t i, uint32_t sx = 0u, uint32_t sy = 0u) {
    return vec2f(corput(i, sx), sobol(i, sy));
  }

  INLINE vec3f sampleSphere(vec2f r) {
    float phi = 2.0 * M_PI * r[0];
    float sinTheta = sqrtf(r[1] * (1.0 - r[1]));
    return vec3f(2.0f * cosf(phi) * sinTheta,
        1.0 - 2.0f * r[1],
        2.0f * sinf(phi) * sinTheta);
  }

  INLINE vec3f cosineSampleHemisphere(vec2f r) {
    const float phi = r[1] * M_PI * 2.0f; 
    const float cosTheta = sqrtf(r[0]);
    const float sinTheta = sqrtf(std::max(0.0f, 1.0f - cosTheta*cosTheta));
    const float cosPhi = cosf(phi);
    const float sinPhi = sinf(phi);
    return vec3f(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta); 
  }

  INLINE vec3f sampleCone(vec2f r, float angle) {
    float cosAngle = cosf(angle / 180.0f * M_PI);
    float d1 = 1 - r[1]*(1 - cosAngle);
    float d0 = cosf(2.0f*M_PI*r[0]) * sqrtf(1 - d1*d1);
    float d2 = sinf(2.0f*M_PI*r[0]) * sqrtf(1 - d1*d1);
    return vec3f(d0, d1, d2);
  }

  INLINE vec3f barycenter(vec2f r) {
    const float r0 = 1.f - sqrt(r.x);
    const float r1 = sqrt(r.x) * (1.f - r.y);
    const float r2 = sqrt(r.x) * r.y;
    return vec3f(r0,r1,r2);
  }

  INLINE vec3f sampleTriangle(vec2f r, vec3f a, vec3f b, vec3f c) {
    const float r0 = r.x;
    const float r1 = (1.f - r0) * r.y;
    const float r2 = 1.f - r0 - r1;
    return r0*a+r1*b+r2*c;
  }

  class Random
  {
  public:
    INLINE Random(const int seed = 27) {setSeed(seed);}

    INLINE void setSeed(const int s)
    {
      const int a = 16807;
      const int m = 2147483647;
      const int q = 127773;
      const int r = 2836;
      int j, k;

      if (s == 0) seed = 1;
      else if (s < 0) seed = -s;
      else seed = s;

      for (j = 32+7; j >= 0; j--) {
        k = seed / q;
        seed = a*(seed - k*q) - r*k;
        if (seed < 0) seed += m;
        if (j < 32) table[j] = seed;
      }
      state = table[0];
    }

    INLINE int getInt()
    {
      const int a = 16807;
      const int m = 2147483647;
      const int q = 127773;
      const int r = 2836;

      int k = seed / q;
      seed = a*(seed - k*q) - r*k;
      if (seed < 0) seed += m;
      int j = state / (1 + (2147483647-1) / 32);
      state = table[j];
      table[j] = seed;

      return state;
    }

    INLINE int getInt(int limit) { return getInt() % limit; }
    INLINE float getFloat () { return min(getInt() / 2147483647.0f, 1.0f - float(ulp)); }
    INLINE double getDouble() { return min(getInt() / 2147483647.0 , 1.0  - double(ulp)); }

  private:
    int seed;
    int state;
    int table[32];
  };
}

#endif /* __RANDOM_HPP__ */


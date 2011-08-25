#ifndef __PF_VEC_HPP__
#define __PF_VEC_HPP__

#include "sys/platform.hpp"
#include "math/math.hpp"

#include <cassert>

#define OP operator
#define DECL template <typename T> INLINE
#define V2 vec2<T>
#define V3 vec3<T>
#define V4 vec4<T>

namespace pf
{
  //////////////////////////////////////////////////////////////////////////////
  // 2D vector
  //////////////////////////////////////////////////////////////////////////////
  template<typename T> struct vec2
  {
    T x, y;
    typedef T scalar;
    enum {N = 2};

    INLINE vec2 () {}
    INLINE vec2 (const vec2& v) {x = v.x; y = v.y;}
    template<typename T1> INLINE vec2 (const vec2<T1>& a) : x(T(a.x)), y(T(a.y)) {}
    template<typename T1> INLINE vec2& operator= (const vec2<T1>& v) {x = v.x; y = v.y; return *this;}
    INLINE explicit vec2(T a) : x(a), y(a) {}
    INLINE explicit vec2(T x, T y) : x(x), y(y) {}
    INLINE explicit vec2(const T *a, index_t stride = 1) : x(a[0]), y(a[stride]) {}
    INLINE vec2(ZeroTy) : x(zero), y(zero) {}
    INLINE vec2(OneTy) : x(one), y(one) {}
    INLINE vec2(PosInfTy) : x(pos_inf), y(pos_inf) {}
    INLINE vec2(NegInfTy) : x(neg_inf), y(neg_inf) {}
    INLINE const T& operator [](size_t axis) const {assert(axis < 2); return (&x)[axis];}
    INLINE       T& operator [](size_t axis)       {assert(axis < 2); return (&x)[axis];}
  };

  DECL V2 OP+ (V2 a)  {return V2(+a.x, +a.y);}
  DECL V2 OP- (V2 a)  {return V2(-a.x, -a.y);}
  DECL V2 abs (V2 a)  {return V2(abs(a.x), abs(a.y));}
  DECL V2 rcp (V2 a)  {return V2(rcp(a.x), rcp(a.y));}
  DECL V2 sqrt (V2 a) {return V2(sqrt(a.x), sqrt(a.y));}
  DECL V2 rsqrt(V2 a) {return V2(rsqrt(a.x), rsqrt(a.y));}
  DECL V2 OP+ (V2 a, V2 b) {return V2(a.x + b.x, a.y + b.y);}
  DECL V2 OP- (V2 a, V2 b) {return V2(a.x - b.x, a.y - b.y);}
  DECL V2 OP* (V2 a, V2 b) {return V2(a.x * b.x, a.y * b.y);}
  DECL V2 OP* (T a, V2 b)  {return V2(a * b.x, a * b.y);}
  DECL V2 OP* (V2 a, T b)  {return V2(a.x * b, a.y * b);}
  DECL V2 OP/ (V2 a, V2 b) {return V2(a.x / b.x, a.y / b.y);}
  DECL V2 OP/ (V2 a, T b)  {return V2(a.x / b, a.y / b);}
  DECL V2 OP/ (T a, V2 b)  {return V2(a / b.x, a / b.y);}
  DECL V2 min(V2 a, V2 b)  {return V2(min(a.x, b.x), min(a.y, b.y));}
  DECL V2 max(V2 a, V2 b)  {return V2(max(a.x, b.x), max(a.y, b.y));}
  DECL T dot(V2 a, V2 b)   {return a.x*b.x + a.y*b.y;}
  DECL T length(V2 a)      {return sqrt(dot(a,a));}
  DECL V2 normalize(V2 a)  {return a*rsqrt(dot(a,a));}
  DECL T distance (V2 a, V2 b) {return length(a-b);}
  DECL V2& OP+= (V2& a, V2 b) {a.x += b.x; a.y += b.y; return a;}
  DECL V2& OP-= (V2& a, V2 b) {a.x -= b.x; a.y -= b.y; return a;}
  DECL V2& OP*= (V2& a, T b)  {a.x *= b; a.y *= b; return a;}
  DECL V2& OP/= (V2& a, T b)  {a.x /= b; a.y /= b; return a;}
  DECL T reduce_add(V2 a) {return a.x + a.y;}
  DECL T reduce_mul(V2 a) {return a.x * a.y;}
  DECL T reduce_min(V2 a) {return min(a.x, a.y);}
  DECL T reduce_max(V2 a) {return max(a.x, a.y);}
  DECL bool OP== (V2 a, V2 b) {return a.x == b.x && a.y == b.y;}
  DECL bool OP!= (V2 a, V2 b) {return a.x != b.x || a.y != b.y;}
  DECL bool OP<  (V2 a, V2 b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return false;
  }
  DECL V2 select (bool s, V2 t, V2 f) {
    return V2(select(s,t.x,f.x),
              select(s,t.y,f.y));
  }
  DECL std::ostream& OP<<(std::ostream& cout, V2 a) {
    return cout << "(" << a.x << ", " << a.y << ")";
  }

  //////////////////////////////////////////////////////////////////////////////
  // 3D vector
  //////////////////////////////////////////////////////////////////////////////
  template<typename T> struct vec3
  {
    T x, y, z;
    typedef T scalar;
    enum {N = 3};

    INLINE vec3 () {}
    INLINE vec3 (const vec3& v) {x = v.x; y = v.y; z = v.z;}
    template<typename T1> INLINE vec3 (const vec3<T1>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)) {}
    template<typename T1> INLINE vec3& OP= (const vec3<T1>& v) {x = v.x; y = v.y; z = v.z; return *this;}
    INLINE explicit vec3 (const T &a) : x(a), y(a), z(a) {}
    INLINE explicit vec3 (const T &x, const T &y, const T &z) : x(x), y(y), z(z) {}
    INLINE explicit vec3 (const T* a, index_t stride = 1) : x(a[0]), y(a[stride]), z(a[2*stride]) {}
    INLINE vec3 (ZeroTy)   : x(zero), y(zero), z(zero) {}
    INLINE vec3 (OneTy)    : x(one), y(one), z(one) {}
    INLINE vec3 (PosInfTy) : x(pos_inf), y(pos_inf), z(pos_inf) {}
    INLINE vec3 (NegInfTy) : x(neg_inf), y(neg_inf), z(neg_inf) {}
    INLINE const T& OP [](size_t axis) const {assert(axis < 3); return (&x)[axis];}
    INLINE       T& OP [](size_t axis)       {assert(axis < 3); return (&x)[axis];}
  };

  DECL V3 abs (V3 a) {return V3(abs(a.x), abs(a.y), abs(a.z));}
  DECL V3 rcp (V3 a) {return V3(rcp(a.x), rcp(a.y), rcp(a.z));}
  DECL V3 sqrt (V3 a) {return V3(sqrt (a.x), sqrt (a.y), sqrt(a.z));}
  DECL V3 rsqrt (V3 a) {return V3(rsqrt(a.x), rsqrt(a.y), rsqrt(a.z));}
  DECL V3 OP+ (V3 a) {return V3(+a.x, +a.y, +a.z);}
  DECL V3 OP- (V3 a) {return V3(-a.x, -a.y, -a.z);}
  DECL V3 OP+ (V3 a, V3 b) {return V3(a.x + b.x, a.y + b.y, a.z + b.z);}
  DECL V3 OP- (V3 a, V3 b) {return V3(a.x - b.x, a.y - b.y, a.z - b.z);}
  DECL V3 OP* (V3 a, V3 b) {return V3(a.x * b.x, a.y * b.y, a.z * b.z);}
  DECL V3 OP/ (V3 a, V3 b) {return V3(a.x / b.x, a.y / b.y, a.z / b.z);}
  DECL V3 OP* (V3 a, T b) {return V3(a.x * b, a.y * b, a.z * b);}
  DECL V3 OP/ (V3 a, T b) {return V3(a.x / b, a.y / b, a.z / b);}
  DECL V3 OP* (T a, V3 b) {return V3(a * b.x, a * b.y, a * b.z);}
  DECL V3 OP/ (T a, V3 b) {return V3(a / b.x, a / b.y, a / b.z);}
  DECL V3 OP+= (V3& a, V3 b) {a.x += b.x; a.y += b.y; a.z += b.z; return a;}
  DECL V3 OP-= (V3& a, V3 b) {a.x -= b.x; a.y -= b.y; a.z -= b.z; return a;}
  DECL V3 OP*= (V3& a, T b) {a.x *= b; a.y *= b; a.z *= b; return a;}
  DECL V3 OP/= (V3& a, T b) {a.x /= b; a.y /= b; a.z /= b; return a;}
  DECL bool OP== (V3 a, V3 b) {return a.x == b.x && a.y == b.y && a.z == b.z;}
  DECL bool OP!= (V3 a, V3 b) {return a.x != b.x || a.y != b.y || a.z != b.z;}
  DECL bool OP<  (V3 a, V3 b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    if (a.z != b.z) return a.z < b.z;
    return false;
  }
  DECL T reduce_add (V3 a) {return a.x + a.y + a.z;}
  DECL T reduce_mul (V3 a) {return a.x * a.y * a.z;}
  DECL T reduce_min (V3 a) {return min(a.x, a.y, a.z);}
  DECL T reduce_max (V3 a) {return max(a.x, a.y, a.z);}
  DECL V3 min (V3 a, V3 b) {return V3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));}
  DECL V3 max (V3 a, V3 b) {return V3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));}
  DECL V3 select (bool s, V3 t, V3 f) {
    return V3(select(s,t.x,f.x),
              select(s,t.y,f.y),
              select(s,t.z,f.z));
  }
  DECL T length (V3 a) {return sqrt(dot(a,a));}
  DECL T dot (V3 a, V3 b) {return a.x*b.x + a.y*b.y + a.z*b.z;}
  DECL T distance (V3 a, V3 b) {return length(a-b);}
  DECL V3 normalize(V3 a) {return a*rsqrt(dot(a,a));}
  DECL V3 cross(V3 a, V3 b) {
    return V3(a.y*b.z - a.z*b.y,
              a.z*b.x - a.x*b.z,
              a.x*b.y - a.y*b.x);
  }
  DECL std::ostream& OP<< (std::ostream& cout, V3 a) {
    return cout << "(" << a.x << ", " << a.y << ", " << a.z << ")";
  }

  //////////////////////////////////////////////////////////////////////////////
  // 4D vector
  //////////////////////////////////////////////////////////////////////////////
  template<typename T> struct vec4
  {
    T x, y, z, w;
    typedef T scalar;
    enum {N = 4};

    INLINE vec4() {}
    INLINE vec4(const vec4& v) {x = v.x; y = v.y; z = v.z; w = v.w;}
    template<typename T1> INLINE vec4 (const vec4<T1>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)), w(T(a.w)) {}
    template<typename T1> INLINE vec4& operator= (const vec4<T1>& v) {x = v.x; y = v.y; z = v.z; w = v.w; return *this;}
    INLINE explicit vec4 (T a) : x(a), y(a), z(a), w(a) {}
    INLINE explicit vec4 (T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    INLINE explicit vec4 (const T *a, index_t stride = 1) : x(a[0]), y(a[stride]), z(a[2*stride]), w(a[3*stride]) {}
    INLINE vec4 (ZeroTy)   : x(zero), y(zero), z(zero), w(zero) {}
    INLINE vec4 (OneTy)    : x(one),  y(one),  z(one),  w(one) {}
    INLINE vec4 (PosInfTy) : x(pos_inf), y(pos_inf), z(pos_inf), w(pos_inf) {}
    INLINE vec4 (NegInfTy) : x(neg_inf), y(neg_inf), z(neg_inf), w(neg_inf) {}
    INLINE const T& operator [](size_t axis) const {assert(axis < 4); return (&x)[axis];}
    INLINE       T& operator [](size_t axis)       {assert(axis < 4); return (&x)[axis];}
  };

  DECL V4 OP+ (V4 a) {return V4(+a.x, +a.y, +a.z, +a.w);}
  DECL V4 OP- (V4 a) {return V4(-a.x, -a.y, -a.z, -a.w);}
  DECL V4 OP+ (V4 a, V4 b) {return V4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);}
  DECL V4 OP- (V4 a, V4 b) {return V4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);}
  DECL V4 OP* (V4 a, V4 b) {return V4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);}
  DECL V4 OP/ (V4 a, V4 b) {return V4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);}
  DECL V4 OP* (V4 a, T b) {return V4(a.x * b, a.y * b, a.z * b, a.w * b);}
  DECL V4 OP/ (V4 a, T b) {return V4(a.x / b, a.y / b, a.z / b, a.w / b);}
  DECL V4 OP* (T a, V4 b) {return V4(a * b.x, a * b.y, a * b.z, a * b.w);}
  DECL V4 OP/ (T a, V4 b) {return V4(a / b.x, a / b.y, a / b.z, a / b.w);}
  DECL V4& OP+= (V4& a, V4 b) {a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a;}
  DECL V4& OP-= (V4& a, V4 b) {a.x -= b.x; a.y -= b.y; a.z -= b.z; a.w -= b.w; return a;}
  DECL V4& OP*= (V4& a, T b) {a.x *= b; a.y *= b; a.z *= b; a.w *= b; return a;}
  DECL V4& OP/= (V4& a, T b) {a.x /= b; a.y /= b; a.z /= b; a.w /= b; return a;}
  DECL bool OP== (V4 a, V4 b) {return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;}
  DECL bool OP!= (V4 a, V4 b) {return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;}
  DECL bool OP<  (V4 a, V4 b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    if (a.z != b.z) return a.z < b.z;
    if (a.w != b.w) return a.w < b.w;
    return false;
  }
  DECL T reduce_add(V4 a) {return a.x + a.y + a.z + a.w;}
  DECL T reduce_mul(V4 a) {return a.x * a.y * a.z * a.w;}
  DECL T reduce_min(V4 a) {return min(a.x, a.y, a.z, a.w);}
  DECL T reduce_max(V4 a) {return max(a.x, a.y, a.z, a.w);}
  DECL V4 min(V4 a, V4 b) {return V4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));}
  DECL V4 max(V4 a, V4 b) {return V4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));}
  DECL V4 abs (V4 a) {return V4(abs(a.x), abs(a.y), abs(a.z), abs(a.w));}
  DECL V4 rcp (V4 a) {return V4(rcp(a.x), rcp(a.y), rcp(a.z), rcp(a.w));}
  DECL V4 sqrt (V4 a) {return V4(sqrt (a.x), sqrt (a.y), sqrt (a.z), sqrt (a.w));}
  DECL V4 rsqrt(V4 a) {return V4(rsqrt(a.x), rsqrt(a.y), rsqrt(a.z), rsqrt(a.w));}
  DECL T length(V4 a) {return sqrt(dot(a,a));}
  DECL T dot (V4 a, V4 b) {return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;}
  DECL V4 normalize(V4 a) {return a*rsqrt(dot(a,a));}
  DECL T distance (V4 a, V4 b) {return length(a-b);}
  DECL V4 select (bool s, V4 t, V4 f) {
    return V4(select(s,t.x,f.x),
              select(s,t.y,f.y),
              select(s,t.z,f.z),
              select(s,t.w,f.w));
  }
  template<typename T> inline std::ostream& OP<<(std::ostream& cout, const V4& a) {
    return cout << "(" << a.x << ", " << a.y << ", " << a.z << ", " << a.w << ")";
  }

  //////////////////////////////////////////////////////////////////////////////
  // Commonly used vector types
  //////////////////////////////////////////////////////////////////////////////
  typedef vec2<bool > vec2b;
  typedef vec2<int  > vec2i;
  typedef vec2<float> vec2f;
  typedef vec3<bool > vec3b;
  typedef vec3<int  > vec3i;
  typedef vec3<float> vec3f;
  typedef vec4<bool > vec4b;
  typedef vec4<int  > vec4i;
  typedef vec4<float> vec4f;
}

#undef OP
#undef DECL
#undef V2
#undef V3
#undef V4

#endif /* __PF_VEC_HPP__ */


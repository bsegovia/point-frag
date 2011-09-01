#ifndef __PF_MATRIX_HPP__
#define __PF_MATRIX_HPP__

#include "vec.hpp"
#include "quaternion.hpp"

namespace pf
{
#define DECL template <typename T> INLINE
#define M33 mat3x3<T>
#define M43 mat4x3<T>
#define M44 mat4x4<T>
#define V3 vec3<T>
#define V4 vec4<T>
#define OP operator

  /////////////////////////////////////////////////////////////////////////////
  // 3x3 Matrix (linear transformation)
  /////////////////////////////////////////////////////////////////////////////
  template<typename T> struct mat3x3
  {
    typedef vec3<T> vector;
    vec3<T> vx,vy,vz;

    /*! Constructors */
    INLINE mat3x3 () {}
    INLINE mat3x3 (const mat3x3 &m) {vx = m.vx; vy = m.vy; vz = m.vz;}
    INLINE mat3x3& OP= (const mat3x3 &m) {vx = m.vx; vy = m.vy; vz = m.vz; return *this;}
    INLINE mat3x3(V3 n) {
      vy = n;
      if (abs(n.x) >= abs(n.y)){
        const float invLength = rcp(sqrt(n.x*n.x + n.z*n.z));
        vx = vec3f(-n.z * invLength, 0.f, n.x * invLength);
      } else {
        const float invLength = rcp(sqrt(n.y*n.y + n.z*n.z));
        vx = vec3f(0.f, n.z * invLength, -n.y * invLength);
      }
      vx = normalize(vx);
      vy = normalize(vy);
      vz = cross(vy,vx);
    }
    template<typename L1> INLINE explicit mat3x3(mat3x3<L1> s) :
      vx(s.vx), vy(s.vy), vz(s.vz) {}
    INLINE mat3x3(V3 vx, V3 vy, V3 vz) : vx(vx), vy(vy), vz(vz) {}
    INLINE mat3x3(QuaternionT<T> q) :
      vx((q.r*q.r + q.i*q.i - q.j*q.j - q.k*q.k),
          2.0f*(q.i*q.j + q.r*q.k),
          2.0f*(q.i*q.k - q.r*q.j)),
      vy(2.0f*(q.i*q.j - q.r*q.k),
         (q.r*q.r - q.i*q.i + q.j*q.j - q.k*q.k),
         2.0f*(q.j*q.k + q.r*q.i)),
      vz(2.0f*(q.i*q.k + q.r*q.j),
         2.0f*(q.j*q.k - q.r*q.i),
         (q.r*q.r - q.i*q.i - q.j*q.j + q.k*q.k)) {}
    INLINE mat3x3(T m00, T m01, T m02,
                  T m10, T m11, T m12,
                  T m20, T m21, T m22) :
      vx(m00,m10,m20), vy(m01,m11,m21), vz(m02,m12,m22) {}
    INLINE mat3x3(ZeroTy) : vx(zero), vy(zero), vz(zero) {}
    INLINE mat3x3(OneTy)  : vx(one, zero, zero), vy(zero, one, zero), vz(zero, zero, one) {}

    /*! Methods */
    INLINE mat3x3 adjoint(void) const {
      return mat3x3(cross(vy,vz),
                    cross(vz,vx),
                    cross(vx,vy)).transposed();
    }
    INLINE mat3x3 transposed(void) const {
      return mat3x3(vx.x,vx.y,vx.z,
                    vy.x,vy.y,vy.z,
                    vz.x,vz.y,vz.z);
    }
    INLINE mat3x3 inverse() const {return rcp(det())*adjoint();}
    INLINE T det() const {return dot(vx,cross(vy,vz));}

    /* Static methods */
    static INLINE mat3x3 scale(V3 &s) {
      return mat3x3(s.x,0,0, 0,s.y,0, 0,0,s.z);
    }
    static INLINE mat3x3 rotate(V3 _u, T r) {
      V3 u = normalize(_u);
      T s = sin(r), c = cos(r);
      return mat3x3(u.x*u.x+(1-u.x*u.x)*c, u.x*u.y*(1-c)-u.z*s,  u.x*u.z*(1-c)+u.y*s,
                    u.x*u.y*(1-c)+u.z*s,   u.y*u.y+(1-u.y*u.y)*c,u.y*u.z*(1-c)-u.x*s,
                    u.x*u.z*(1-c)-u.y*s,   u.y*u.z*(1-c)+u.x*s,  u.z*u.z+(1-u.z*u.z)*c);
   }
 };

  /*! External operators */
  DECL M33 OP- (M33 a) {return M33(-a.vx,-a.vy,-a.vz);}
  DECL M33 OP+ (M33 a) {return M33(+a.vx,+a.vy,+a.vz);}
  DECL M33 rcp (M33 a) {return a.inverse();}
  DECL M33 OP+ (M33 a, M33 b) {return M33(a.vx+b.vx,a.vy+b.vy,a.vz+b.vz);}
  DECL M33 OP- (M33 a, M33 b) {return M33(a.vx-b.vx,a.vy-b.vy,a.vz-b.vz);}
  DECL M33 OP* (M33 a, M33 b) {return M33(a*b.vx, a*b.vy, a*b.vz);}
  DECL M33 OP/ (M33 a, M33 b) {return a * rcp(b);}
  DECL V3  OP* (M33 a, V3 b) {return b.x*a.vx + b.y*a.vy + b.z*a.vz;}
  DECL M33 OP* (T a, M33 b) {return M33(a*b.vx, a*b.vy, a*b.vz);}
  DECL M33 OP/ (M33 a, T b) {return a * rcp(b);}
  DECL M33& OP/= (M33& a, T b) {return a = a / b;}
  DECL M33& OP*= (M33& a, T b) {return a = a * b;}
  DECL M33& OP*= (M33& a, M33 b) {return a = a * b;}
  DECL M33& OP/= (M33& a, M33 b) {return a = a / b;}
  DECL bool OP== (M33 a, M33 b) {return (a.vx==b.vx) && (a.vy == b.vy) && (a.vz == b.vz);}
  DECL bool OP!= (M33 a, M33 b) {return (a.vx!=b.vx) || (a.vy != b.vy) || (a.vz != b.vz);}

  /*! External functions */
  DECL V3 xfmPoint (M33 s, V3 a) {return a.x*s.vx + a.y*s.vy + a.z*s.vz;}
  DECL V3 xfmvector(M33 s, V3 a) {return a.x*s.vx + a.y*s.vy + a.z*s.vz;}
  DECL V3 xfmNormal(M33 s, V3 a) {return xfmvector(s.inverse().transposed(),a);}
  DECL M33 frame(V3 N) {
    V3 dx0 = cross(V3(T(one),T(zero),T(zero)),N);
    V3 dx1 = cross(V3(T(zero),T(one),T(zero)),N);
    V3 dx = normalize(select(dot(dx0,dx0) > dot(dx1,dx1),dx0,dx1));
    V3 dy = normalize(cross(N,dx));
    return M33(dx,dy,N);
  }
  template<typename T> static std::ostream& OP<<(std::ostream& cout, const M33& m) {
    return cout << "{vx = " << m.vx << ", vy = " << m.vy << ", vz = " << m.vz << "}";
  }

  ////////////////////////////////////////////////////////////////////////////
  // 4x3 Matrix (affine transformation)
  ////////////////////////////////////////////////////////////////////////////
  template<typename T>
  struct mat4x3
  {
    M33 l; /*! linear part of affine space */
    V3 p;  /*! affine part of affine space */

    /*! Constructors */
    INLINE mat4x3 (){}
    INLINE mat4x3 (const mat4x3& m) {l = m.l; p = m.p;}
    INLINE mat4x3 (V3 vx, V3 vy, V3 vz, V3 p) : l(vx,vy,vz), p(p) {}
    INLINE mat4x3 (M33 l, V3 p) : l(l), p(p) {}

    INLINE mat4x3& OP= (const mat4x3 &m) {l = m.l; p = m.p; return *this;}
    template<typename L1> INLINE explicit mat4x3(const mat4x3<L1>& s) : l(s.l), p(s.p) {}
    INLINE mat4x3(ZeroTy) : l(zero), p(zero) {}
    INLINE mat4x3(OneTy)  : l(one),  p(zero) {}

    /*! Static methods */
    static INLINE mat4x3 scale(V3 s) {return mat4x3(M33::scale(s),zero);}
    static INLINE mat4x3 translate(V3 p) {return mat4x3(one,p);}
    static INLINE mat4x3 rotate(V3 u, T r) {return mat4x3(M33::rotate(u,r),zero);}
    static INLINE mat4x3 rotate(V3 p, V3 u, T r) {return translate(p)*rotate(u,r)*translate(-p);}
    static INLINE mat4x3 lookAtPoint(V3 eye, V3 point, V3 up) {
      V3 z = normalize(point-eye);
      V3 u = normalize(cross(up,z));
      V3 v = normalize(cross(z,u));
      return mat4x3(M33(u,v,z),eye);
    }
  };

  /*! External operators */
  DECL M43 OP- (M43 a) {return M43(-a.l,-a.p);}
  DECL M43 OP+ (M43 a) {return M43(+a.l,+a.p);}
  DECL M43 rcp (M43 a) {M33 il = rcp(a.l); return M43(il,-il*a.p);}
  DECL M43 OP+ (M43 a, M43 b)  {return M43(a.l+b.l,a.p+b.p);}
  DECL M43 OP- (M43 a, M43 b)  {return M43(a.l-b.l,a.p-b.p);}
  DECL M43 OP* (T a,   M43 b)  {return M43(a*b.l,a*b.p);}
  DECL M43 OP* (M43 a, M43 b)  {return M43(a.l*b.l,a.l*b.p+a.p);}
  DECL M43 OP/ (M43 a, M43 b)  {return a * rcp(b);}
  DECL M43 OP/ (M43 a, T b)    {return a * rcp(b);}
  DECL M43& OP*= (M43& a, T b) {return a = a*b;}
  DECL M43& OP/= (M43& a, T b) {return a = a/b;}
  DECL M43& OP*= (M43& a, M43 b) {return a = a*b;}
  DECL M43& OP/= (M43& a, M43 b) {return a = a/b;}
  DECL bool OP== (M43 a, M43 b)  {return (a.l==b.l) && (a.p==b.p);}
  DECL bool OP!= (M43 a, M43 b)  {return (a.l!=b.l) || (a.p!=b.p);}

  /*! External functions */
  DECL V3 xfmPoint (M43 m, V3 p) {return xfmPoint(m.l,p) + m.p;}
  DECL V3 xfmvector(M43 m, V3 v) {return xfmvector(m.l,v);}
  DECL V3 xfmNormal(M43 m, V3 n) {return xfmNormal(m.l,n);}
  template<typename T> static std::ostream& OP<<(std::ostream& cout, M43 m) {
    return cout << "{l = " << m.l << ", p = " << m.p << "}";
  }

  ////////////////////////////////////////////////////////////////////////////
  // 4x4 Matrix (homogenous transformation)
  ////////////////////////////////////////////////////////////////////////////
  template<typename T>
  struct mat4x4
  {
    V4 c[4]; /*! Column major storage */

    /*! Constructors */
    INLINE mat4x4(void) {}
    INLINE mat4x4(OneTy) {
      c[0] = V4(one, zero, zero, zero);
      c[1] = V4(zero, one, zero, zero);
      c[2] = V4(zero, zero, one, zero);
      c[3] = V4(zero, zero, zero, one);
    }
    INLINE mat4x4(ZeroTy) {c[0] = c[1] = c[2] = c[3] = zero;}
    INLINE mat4x4(const M44 &m) {
      c[0] = m.c[0]; c[1] = m.c[1]; c[2] = m.c[2]; c[3] = m.c[3];
    }
    INLINE mat4x4(T s) {
      c[0] = V4(s, zero, zero, zero);
      c[1] = V4(zero, s, zero, zero);
      c[2] = V4(zero, zero, s, zero);
      c[3] = V4(zero, zero, zero, s);
    }
    INLINE mat4x4(T x0, T y0, T z0, T w0,
                  T x1, T y1, T z1, T w1,
                  T x2, T y2, T z2, T w2,
                  T x3, T y3, T z3, T w3) {
      c[0] = V4(x0, y0, z0, w0);
      c[1] = V4(x1, y1, z1, w1);
      c[2] = V4(x2, y2, z2, w2);
      c[3] = V4(x3, y3, z3, w3);
    }
    INLINE mat4x4(V4 v0, V4 v1, V4 v2, V4 v3) {
      c[0] = v0; c[1] = v1; c[2] = v2; c[3] = v3;
    }
    template <typename U> INLINE mat4x4 (const mat4x4<U> &m) {
      c[0] = V4(m[0]);
      c[1] = V4(m[1]);
      c[2] = V4(m[2]);
      c[3] = V4(m[3]);
    }
    template <typename U> INLINE mat4x4 (U s) {
      c[0] = V4(T(s), zero, zero, zero);
      c[1] = V4(zero, T(s), zero, zero);
      c[2] = V4(zero, zero, T(s), zero);
      c[3] = V4(zero, zero, zero, T(s));
    }
    INLINE mat4x4 (M33 m) {
      c[0] = V4(m[0], zero);
      c[1] = V4(m[1], zero);
      c[2] = V4(m[2], zero);
      c[3] = V4(zero, zero, zero, one);
    }
    INLINE mat4x4 (M43 m) {
      c[0] = V4(m[0], zero);
      c[1] = V4(m[1], zero);
      c[2] = V4(m[2], zero);
      c[3] = V4(m[3], one);
    }

    /*! Methods and operators */
#define DECL_U template <typename U> INLINE
#define M44U const mat4x4<U>& 
    DECL_U M44& OP+= (U s) {c[0]+=s; c[1]+=s; c[2]+=s; c[3]+=s; return *this;}
    DECL_U M44& OP-= (U s) {c[0]-=s; c[1]-=s; c[2]-=s; c[3]-=s; return *this;}
    DECL_U M44& OP*= (U s) {c[0]*=s; c[1]*=s; c[2]*=s; c[3]*=s; return *this;}
    DECL_U M44& OP/= (U s) {c[0]/=s; c[1]/=s; c[2]/=s; c[3]/=s; return *this;}
    DECL_U M44& OP=  (M44U m) {c[0]=m[0];  c[1]=m[1];  c[2]=m[2];  c[3]=m[3]; return *this;}
    DECL_U M44& OP+= (M44U m) {c[0]+=m[0]; c[1]+=m[1]; c[2]+=m[2]; c[3]+=m[3]; return *this;}
    DECL_U M44& OP-= (M44U m) {c[0]-=m[0]; c[1]-=m[1]; c[2]-=m[2]; c[3]-=m[3]; return *this;}
    DECL_U M44& OP*= (M44U m) {c[0]*=m[0]; c[1]*=m[1]; c[2]*=m[2]; c[3]*=m[3]; return *this;}
    DECL_U M44& OP/= (M44U m) {c[0]/=m[0]; c[1]/=m[1]; c[2]/=m[2]; c[3]/=m[3]; return *this;}
#undef DECL_U
#undef M44U
    M44& OP= (const M44 &m) {c[0]=m[0];c[1]=m[1];c[2]=m[2];c[3]=m[3];return *this;}

    INLINE M44 inverse() const {
      const T f00 = c[2][2]*c[3][3] - c[3][2]*c[2][3];
      const T f01 = c[2][1]*c[3][3] - c[3][1]*c[2][3];
      const T f02 = c[2][1]*c[3][2] - c[3][1]*c[2][2];
      const T f03 = c[2][0]*c[3][3] - c[3][0]*c[2][3];
      const T f04 = c[2][0]*c[3][2] - c[3][0]*c[2][2];
      const T f05 = c[2][0]*c[3][1] - c[3][0]*c[2][1];
      const T f06 = c[1][2]*c[3][3] - c[3][2]*c[1][3];
      const T f07 = c[1][1]*c[3][3] - c[3][1]*c[1][3];
      const T f08 = c[1][1]*c[3][2] - c[3][1]*c[1][2];
      const T f09 = c[1][0]*c[3][3] - c[3][0]*c[1][3];
      const T f10 = c[1][0]*c[3][2] - c[3][0]*c[1][2];
      const T f11 = c[1][1]*c[3][3] - c[3][1]*c[1][3];
      const T f12 = c[1][0]*c[3][1] - c[3][0]*c[1][1];
      const T f13 = c[1][2]*c[2][3] - c[2][2]*c[1][3];
      const T f14 = c[1][1]*c[2][3] - c[2][1]*c[1][3];
      const T f15 = c[1][1]*c[2][2] - c[2][1]*c[1][2];
      const T f16 = c[1][0]*c[2][3] - c[2][0]*c[1][3];
      const T f17 = c[1][0]*c[2][2] - c[2][0]*c[1][2];
      const T f18 = c[1][0]*c[2][1] - c[2][0]*c[1][1];
      M44 inv(+c[1][1]*f00 - c[1][2]*f01 + c[1][3]*f02,
              -c[1][0]*f00 + c[1][2]*f03 - c[1][3]*f04,
              +c[1][0]*f01 - c[1][1]*f03 + c[1][3]*f05,
              -c[1][0]*f02 + c[1][1]*f04 - c[1][2]*f05,
              -c[0][1]*f00 + c[0][2]*f01 - c[0][3]*f02,
              +c[0][0]*f00 - c[0][2]*f03 + c[0][3]*f04,
              -c[0][0]*f01 + c[0][1]*f03 - c[0][3]*f05,
              +c[0][0]*f02 - c[0][1]*f04 + c[0][2]*f05,
              +c[0][1]*f06 - c[0][2]*f07 + c[0][3]*f08,
              -c[0][0]*f06 + c[0][2]*f09 - c[0][3]*f10,
              +c[0][0]*f11 - c[0][1]*f09 + c[0][3]*f12,
              -c[0][0]*f08 + c[0][1]*f10 - c[0][2]*f12,
              -c[0][1]*f13 + c[0][2]*f14 - c[0][3]*f15,
              +c[0][0]*f13 - c[0][2]*f16 + c[0][3]*f17,
              -c[0][0]*f14 + c[0][1]*f16 - c[0][3]*f18,
              +c[0][0]*f15 - c[0][1]*f17 + c[0][2]*f18);
      const T det = c[0][0]*inv[0][0] + c[0][1]*inv[1][0] + c[0][2]*inv[2][0] + c[0][3]*inv[3][0];
      inv /= det;
      return inv;
    }
    INLINE V4&        OP[] (int i) {return this->c[i];}
    INLINE const V4&  OP[] (int i) const {return this->c[i];}
  };

  /*! External operators */
  DECL M44 OP+ (M44 m, T s) {return M44(m[0]+s, m[1]+s, m[2]+s, m[3]+s);}
  DECL M44 OP+ (T s, M44 m) {return M44(m[0]+s, m[1]+s, m[2]+s, m[3]+s);}
  DECL M44 OP- (M44 m, T s) {return M44(m[0]-s, m[1]-s, m[2]-s, m[3]-s);}
  DECL M44 OP- (T s, M44 m) {return M44(m[0]-s, m[1]-s, m[2]-s, m[3]-s);}
  DECL M44 OP* (M44 m, T s) {return M44(m[0]*s, m[1]*s, m[2]*s, m[3]*s);}
  DECL M44 OP* (T s, M44 m) {return M44(m[0]*s, m[1]*s, m[2]*s, m[3]*s);}
  DECL M44 OP/ (M44 m, T s) {return M44(m[0]/s, m[1]/s, m[2]/s, m[3]/s);}
  DECL M44 OP+ (M44 m1, M44 m2) {return M44(m1[0]+m2[0], m1[1]+m2[1], m1[2]+m2[2], m1[3]+m2[3]);}
  DECL M44 OP- (M44 m1, M44 m2) {return M44(m1[0]-m2[0], m1[1]-m2[1], m1[2]-m2[2], m1[3]-m2[3]);}
  DECL V4  OP* (M44 m, V4 v) {
    return V(m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]*v.w,
             m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]*v.w,
             m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]*v.w,
             m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]*v.w);
  }
  DECL V4 OP* (V4 v, M44 m) {
    return V(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
             m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
             m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
             m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w);
  }
  DECL M44 OP* (M44 m1, M44 m2) {
    const V4 a0 = m1[0], a1 = m1[1], a2 = m1[2], a3 = m1[3];
    const V4 b0 = m2[0], b1 = m2[1], b2 = m2[2], b3 = m2[3];
    M44 m;
    m[0] = a0*b0[0] + a1*b0[1] + a2*b0[2] + a3*b0[3];
    m[1] = a0*b1[0] + a1*b1[1] + a2*b1[2] + a3*b1[3];
    m[2] = a0*b2[0] + a1*b2[1] + a2*b2[2] + a3*b2[3];
    m[3] = a0*b3[0] + a1*b3[1] + a2*b3[2] + a3*b3[3];
    return m;
  }
  DECL V4 OP/ (M44 m, V4 v) {return m._inverse() * v;}
  DECL V4 OP/ (V4 v, M44 m) {return v * m._inverse();}
  DECL M44 OP/ (M44 m1, M44 m2) {return m1 * m2._inverse();}
  DECL M44 OP- (M44 m) {return M44(-m[0], -m[1], -m[2], -m[3]); }
  DECL bool OP== (M44 m, M44 n) {return (m[0]==n[0]) && (m[1]==n[1]) && (m[2]==n[2]) && (m[3]==n[3]);}
  DECL bool OP!= (M44 m, M44 n) {return (m[0]!=n[0]) || (m[1]!=n[1]) || (m[2]!=n[2]) || (m[3]!=n[3]);}

  /*! External functions */
  DECL M44 translate (M44 m, V3 v) {
    M44 dst(m);
    dst[3] = m[0]*v[0] + m[1]*v[1] + m[2]*v[2] + m[3];
    return dst;
  }
  DECL M44 lookAt (V3 eye, V3 center, V3 up) {
    M44 dst(one);
    V3 f = normalize(center - eye);
    V3 u = normalize(up);
    V3 s = normalize(cross(f, u));
    u = cross(s, f);
    dst[0][0] = s.x; dst[1][0] = s.y; dst[2][0] = s.z;
    dst[0][1] = u.x; dst[1][1] = u.y; dst[2][1] = u.z;
    dst[0][2] =-f.x; dst[1][2] =-f.y; dst[2][2] =-f.z;
    return translate(dst, -eye);
  }
  DECL M44 perspective (T fovy, T aspect, T zNear, T zFar) {
    M44 dst(zero);
    T range = tan(deg2rad(fovy / T(2.f))) * zNear;	
    T left = -range * aspect;
    T right = range * aspect;
    T bottom = -range;
    T top = range;
    dst[0][0] = (T(2) * zNear) / (right - left);
    dst[1][1] = (T(2) * zNear) / (top - bottom);
    dst[2][2] = - (zFar + zNear) / (zFar - zNear);
    dst[2][3] = - T(1);
    dst[3][2] = - (T(2) * zFar * zNear) / (zFar - zNear);
    return dst;
  }
  DECL M44 rotate (M44 m, T angle, V3 v) {
    M44 rot(zero), dst(zero);
    const T a = deg2rad(angle);
    const T c = cos(a);
    const T s = sin(a);
    const V3 axis = normalize(v);
    const V3 temp = (T(1) - c) * axis;
    rot[0][0] = c + temp[0]*axis[0];
    rot[0][1] = 0 + temp[0]*axis[1] + s*axis[2];
    rot[0][2] = 0 + temp[0]*axis[2] - s*axis[1];
    rot[1][0] = 0 + temp[1]*axis[0] - s*axis[2];
    rot[1][1] = c + temp[1]*axis[1];
    rot[1][2] = 0 + temp[1]*axis[2] + s*axis[0];
    rot[2][0] = 0 + temp[2]*axis[0] + s*axis[1];
    rot[2][1] = 0 + temp[2]*axis[1] - s*axis[0];
    rot[2][2] = c + temp[2]*axis[2];
    dst[0] = m[0]*rot[0][0] + m[1]*rot[0][1] + m[2]*rot[0][2];
    dst[1] = m[0]*rot[1][0] + m[1]*rot[1][1] + m[2]*rot[1][2];
    dst[2] = m[0]*rot[2][0] + m[1]*rot[2][1] + m[2]*rot[2][2];
    dst[3] = m[3];
    return dst;
  }

  template<typename T> static std::ostream& OP<<(std::ostream& cout, M44 m) {
    return cout << "{ c0 = " << m.c[0] <<
                   ", c1 = " << m.c[1] <<
                   ", c2 = " << m.c[2] <<
                   ", c3 = " << m.c[3] << " };";
  }
#if 0

  template <typename T> 
    detail::mat4x4<T> scale
    (
     detail::mat4x4<T> const & m,
     detail::tvec3<T> const & v
    )
    {
      detail::mat4x4<T> Result(detail::mat4x4<T>::null);
      Result[0] = m[0] * v[0];
      Result[1] = m[1] * v[1];
      Result[2] = m[2] * v[2];
      Result[3] = m[3];
      return Result;
    }

  template <typename T> 
    detail::mat4x4<T> translate_slow
    (
     detail::mat4x4<T> const & m,
     detail::tvec3<T> const & v
    )
    {
      detail::mat4x4<T> Result(T(1));
      Result[3] = detail::tvec4<T>(v, T(1));
      return m * Result;

      //detail::mat4x4<valType> Result(m);
      Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
      //Result[3][0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
      //Result[3][1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
      //Result[3][2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
      //Result[3][3] = m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3];
      //return Result;
    }

  template <typename T> 
    detail::mat4x4<T> rotate_slow
    (
     detail::mat4x4<T> const & m,
     T const & angle, 
     detail::tvec3<T> const & v
    )
    {
      T a = radians(angle);
      T c = cos(a);
      T s = sin(a);
      detail::mat4x4<T> Result;

      detail::tvec3<T> axis = normalize(v);

      Result[0][0] = c + (1 - c)      * axis.x     * axis.x;
      Result[0][1] = (1 - c) * axis.x * axis.y + s * axis.z;
      Result[0][2] = (1 - c) * axis.x * axis.z - s * axis.y;
      Result[0][3] = 0;

      Result[1][0] = (1 - c) * axis.y * axis.x - s * axis.z;
      Result[1][1] = c + (1 - c) * axis.y * axis.y;
      Result[1][2] = (1 - c) * axis.y * axis.z + s * axis.x;
      Result[1][3] = 0;

      Result[2][0] = (1 - c) * axis.z * axis.x + s * axis.y;
      Result[2][1] = (1 - c) * axis.z * axis.y - s * axis.x;
      Result[2][2] = c + (1 - c) * axis.z * axis.z;
      Result[2][3] = 0;

      Result[3] = detail::tvec4<T>(0, 0, 0, 1);
      return m * Result;
    }

  template <typename T> 
    detail::mat4x4<T> scale_slow
    (
     detail::mat4x4<T> const & m,
     detail::tvec3<T> const & v
    )
    {
      detail::mat4x4<T> Result(T(1));
      Result[0][0] = v.x;
      Result[1][1] = v.y;
      Result[2][2] = v.z;
      return m * Result;
    }

  template <typename valType> 
    detail::mat4x4<valType> ortho
    (
     valType const & left, 
     valType const & right, 
     valType const & bottom, 
     valType const & top, 
     valType const & zNear, 
     valType const & zFar
    )
    {
      detail::mat4x4<valType> Result(1);
      Result[0][0] = valType(2) / (right - left);
      Result[1][1] = valType(2) / (top - bottom);
      Result[2][2] = - valType(2) / (zFar - zNear);
      Result[3][0] = - (right + left) / (right - left);
      Result[3][1] = - (top + bottom) / (top - bottom);
      Result[3][2] = - (zFar + zNear) / (zFar - zNear);
      return Result;
    }

  template <typename valType> 
    detail::mat4x4<valType> ortho(
        valType const & left, 
        valType const & right, 
        valType const & bottom, 
        valType const & top)
    {
      detail::mat4x4<valType> Result(1);
      Result[0][0] = valType(2) / (right - left);
      Result[1][1] = valType(2) / (top - bottom);
      Result[2][2] = - valType(1);
      Result[3][0] = - (right + left) / (right - left);
      Result[3][1] = - (top + bottom) / (top - bottom);
      return Result;
    }

  template <typename valType> 
    detail::mat4x4<valType> frustum
    (
     valType const & left, 
     valType const & right, 
     valType const & bottom, 
     valType const & top, 
     valType const & nearVal, 
     valType const & farVal
    )
    {
      detail::mat4x4<valType> Result(0);
      Result[0][0] = (valType(2) * nearVal) / (right - left);
      Result[1][1] = (valType(2) * nearVal) / (top - bottom);
      Result[2][0] = (right + left) / (right - left);
      Result[2][1] = (top + bottom) / (top - bottom);
      Result[2][2] = -(farVal + nearVal) / (farVal - nearVal);
      Result[2][3] = valType(-1);
      Result[3][2] = -(valType(2) * farVal * nearVal) / (farVal - nearVal);
      return Result;
    }

  template <typename valType>
    detail::mat4x4<valType> perspectiveFov
    (
     valType const & fov, 
     valType const & width, 
     valType const & height, 
     valType const & zNear, 
     valType const & zFar
    )
    {
      valType rad = glm::radians(fov);
      valType h = glm::cos(valType(0.5) * rad) / glm::sin(valType(0.5) * rad);
      valType w = h * height / width;

      detail::mat4x4<valType> Result(valType(0));
      Result[0][0] = w;
      Result[1][1] = h;
      Result[2][2] = (zFar + zNear) / (zFar - zNear);
      Result[2][3] = valType(1);
      Result[3][2] = -(valType(2) * zFar * zNear) / (zFar - zNear);
      return Result;
    }

  template <typename T> 
    detail::mat4x4<T> infinitePerspective
    (
     T fovy, 
     T aspect, 
     T zNear
    )
    {
      T range = tan(radians(fovy / T(2))) * zNear;	
      T left = -range * aspect;
      T right = range * aspect;
      T bottom = -range;
      T top = range;

      detail::mat4x4<T> Result(T(0));
      Result[0][0] = (T(2) * zNear) / (right - left);
      Result[1][1] = (T(2) * zNear) / (top - bottom);
      Result[2][2] = - T(1);
      Result[2][3] = - T(1);
      Result[3][2] = - T(2) * zNear;
      return Result;
    }

  template <typename T> 
    detail::mat4x4<T> tweakedInfinitePerspective
    (
     T fovy, 
     T aspect, 
     T zNear
    )
    {
      T range = tan(radians(fovy / T(2))) * zNear;	
      T left = -range * aspect;
      T right = range * aspect;
      T bottom = -range;
      T top = range;

      detail::mat4x4<T> Result(T(0));
      Result[0][0] = (T(2) * zNear) / (right - left);
      Result[1][1] = (T(2) * zNear) / (top - bottom);
      Result[2][2] = T(0.0001) - T(1);
      Result[2][3] = T(-1);
      Result[3][2] = - (T(0.0001) - T(2)) * zNear;
      return Result;
    }

  template <typename T, typename U>
    detail::tvec3<T> project
    (
     detail::tvec3<T> const & obj, 
     detail::mat4x4<T> const & model, 
     detail::mat4x4<T> const & proj, 
     detail::tvec4<U> const & viewport
    )
    {
      detail::tvec4<T> tmp = detail::tvec4<T>(obj, T(1));
      tmp = model * tmp;
      tmp = proj * tmp;

      tmp /= tmp.w;
      tmp = tmp * T(0.5) + T(0.5);
      tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
      tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

      return detail::tvec3<T>(tmp);
    }

  template <typename T, typename U>
    detail::tvec3<T> unProject
    (
     detail::tvec3<T> const & win, 
     detail::mat4x4<T> const & model, 
     detail::mat4x4<T> const & proj, 
     detail::tvec4<U> const & viewport
    )
    {
      detail::mat4x4<T> inverse = glm::inverse(proj * model);

      detail::tvec4<T> tmp = detail::tvec4<T>(win, T(1));
      tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
      tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
      tmp = tmp * T(2) - T(1);

      detail::tvec4<T> obj = inverse * tmp;
      obj /= obj.w;

      return detail::tvec3<T>(obj);
    }

  template <typename T, typename U> 
    detail::mat4x4<T> pickMatrix
    (
     detail::tvec2<T> const & center, 
     detail::tvec2<T> const & delta, 
     detail::tvec4<U> const & viewport
    )
    {
      assert(delta.x > T(0) && delta.y > T(0));
      detail::mat4x4<T> Result(1.0f);

      if(!(delta.x > T(0) && delta.y > T(0))) 
        return Result; // Error

      detail::tvec3<T> Temp(
          (T(viewport[2]) - T(2) * (center.x - T(viewport[0]))) / delta.x,
          (T(viewport[3]) - T(2) * (center.y - T(viewport[1]))) / delta.y,
          T(0));

      // Translate and scale the picked region to the entire window
      Result = translate(Result, Temp);
      return scale(Result, detail::tvec3<T>(T(viewport[2]) / delta.x, T(viewport[3]) / delta.y, T(1)));
    }
#endif

  /////////////////////////////////////////////////////////////////////////////
  // Commonly used matrix types
  /////////////////////////////////////////////////////////////////////////////
  typedef mat3x3<float> mat3x3f;
  typedef mat3x3<double> mat3x3d;
  typedef mat4x3<mat3x3f> mat4x3f;
  typedef mat4x4<float> mat4x4f;
  typedef mat4x4<double> mat4x4d;

#undef DECL
#undef M33
#undef M43
#undef M44
#undef V3
#undef V4
#undef OP
}

#endif /* __PF_MATRIX_HPP__ */


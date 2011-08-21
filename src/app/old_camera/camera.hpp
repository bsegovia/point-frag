#ifndef __PF_CAMERA_HPP__
#define __PF_CAMERA_HPP__

#include "math/matrix.hpp"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

namespace pf
{
  /*! Simple FPS like camera */
  struct FlyCamera {
  public:
    FlyCamera(const vec3f &pos_ = vec3f(0.f,1.f,-2.f),
              const vec3f &up_ = vec3f(0.f,-1.f,0.f),
              const vec3f &view_ = vec3f(0.f,0.f,1.f),
              float fov_ = 45.f,
              float ratio_ = 1.f,
              float near_ = 0.01f,
              float far_ = 100.f);
    /*! Update the orientation of the camera */
    void updateOrientation(float dx, float dy);
    /*! Update positions along x, y and z axis */
    void updatePosition(const vec3f &d);
    /*! Return the GL view matrix for the given postion */
    INLINE mat4x4f getMatrix(void) {
      const mat4x4f P = pf::perspective(fov, ratio, near, far);
      const mat4x4f V = pf::lookAt(pos, lookAt, up);
      return P*V;
    }
    INLINE mat4x4f getMatrix2(void) {
      using namespace glm::gtc;
#define HOP(DST, SRC)\
      glm::vec3 DST;\
      DST[0] = SRC.x;\
      DST[1] = SRC.y;\
      DST[2] = SRC.z;
      HOP(pos_, pos);
      HOP(up_, up);
      HOP(lookAt_, lookAt);
      const glm::mat4 P = glm::perspective(fov, ratio, near, far);
      const glm::mat4 V = matrix_transform::lookAt(pos_, lookAt_, up_);
      glm::mat4 d = P * V;
      mat4x4f m;
      for (int i =0; i < 4; ++i)
      for (int j =0; j < 4; ++j)
        m[i][j] = d[i][j];
      return m;
    }
    vec3f pos, up, view, lookAt;
    float fov, ratio, near, far;
  };
}

#endif /* __PF_CAMERA_HPP__ */


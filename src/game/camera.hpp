#ifndef __PF_CAMERA_HPP__
#define __PF_CAMERA_HPP__

#include "math/matrix.hpp"
#include "sys/tasking.hpp"

namespace pf
{
  /*! Simple FPS like camera */
  class FlyCamera : public RefCount
  {
  public:
    FlyCamera(const vec3f &pos_ = vec3f(0.f,-1.f,-2.f),
              const vec3f &up_ = vec3f(0.f,-1.f,0.f),
              const vec3f &view_ = vec3f(0.f,0.f,1.f),
              float fov_ = 45.f,
              float ratio_ = 1.f,
              float near_ = 0.1f,
              float far_ = 10000.f);
    FlyCamera(const FlyCamera &other);
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
    vec3f pos, up, view, lookAt;
    float fov, ratio, near, far;
    float speed;           //!< "Translation" speed [m/s]
    float angularSpeed;    //!< "Rotation" speed [radian/pixel/s]
    static const float defaultSpeed;
    static const float defaultAngularSpeed;
    static const float acosMinAngle;
  };

  /*! Handle mouse, keyboard ... */
  class InputEvent;

  /*! Update the camera */
  class TaskCamera : public Task
  {
  public:
    TaskCamera(FlyCamera &cam, InputEvent &event);
    virtual Task *run(void);
    Ref<FlyCamera> cam;    //!< Camera to update
    Ref<InputEvent> event; //!< Current event state
  };
}

#endif /* __PF_CAMERA_HPP__ */


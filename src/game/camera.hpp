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

#ifndef __PF_CAMERA_HPP__
#define __PF_CAMERA_HPP__

#include "math/matrix.hpp"
#include "sys/tasking.hpp"

namespace pf
{
  /*! Simple FPS like camera */
  class FPSCamera : public RefCount, public NonCopyable
  {
  public:
    FPSCamera(const vec3f &org = vec3f(0.f,0.4f,1.2f),
              const vec3f &up = vec3f(0.f,1.f,0.f),
              const vec3f &view = vec3f(0.f,0.f,-1.f),
              float fov = 80.f,
              float ratio = 1.f,
              float znear = 0.1f,
              float zfar = 10000.f);
    FPSCamera(const FPSCamera &other);
    /*! Update the orientation of the camera */
    void updateOrientation(float dx, float dy);
    /*! Update positions along x, y and z axis */
    void updatePosition(const vec3f &d);
    /*! Return the GL view matrix for the given postion XXX Remove it */
    INLINE mat4x4f getMatrix(void) {
      const mat4x4f P = pf::perspective(fov, ratio, znear, zfar);
      const mat4x4f V = pf::lookAt(org, lookAt, up);
      return P*V;
    }
    vec3f org, up, view, lookAt;
    float fov, ratio, znear, zfar;
    float speed;           //!< "Translation" speed [m/s]
    float angularSpeed;    //!< "Rotation" speed [radian/pixel]
    static const float defaultSpeed;
    static const float defaultAngularSpeed;
    static const float acosMinAngle;
    PF_CLASS(FPSCamera);
  };

  /*! Handle mouse, keyboard ... */
  class InputEvent;

  /*! Update the camera */
  class TaskCamera : public Task
  {
  public:
    TaskCamera(FPSCamera &cam, InputEvent &event);
    virtual Task *run(void);
    Ref<FPSCamera> cam;    //!< Camera to update
    Ref<InputEvent> event; //!< Current event state
  };
}

#endif /* __PF_CAMERA_HPP__ */


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

#include "camera.hpp"
#include "game_event.hpp"
#include "sys/logging.hpp"
#include "sys/windowing.hpp"

namespace pf
{
  const float FPSCamera::defaultSpeed = 5.f;
  const float FPSCamera::defaultAngularSpeed = 4.f * 180.f / float(pi) / 50000.f;
  const float FPSCamera::acosMinAngle = 0.95f;

  FPSCamera::FPSCamera(const vec3f &org,
                       const vec3f &up,
                       const vec3f &view,
                       float fov,
                       float ratio,
                       float znear,
                       float zfar) :
    org(org), up(up), view(view), lookAt(org+view),
    fov(fov), ratio(ratio), znear(znear), zfar(zfar),
    speed(defaultSpeed), angularSpeed(defaultAngularSpeed)
  {}

  FPSCamera::FPSCamera(const FPSCamera &other) {
    this->org = other.org;
    this->up = other.up;
    this->view = other.view;
    this->lookAt = other.lookAt;
    this->fov = other.fov;
    this->ratio = other.ratio;
    this->znear = other.znear;
    this->zfar = other.zfar;
    this->speed = other.speed;
    this->angularSpeed = other.angularSpeed;
  }

  void FPSCamera::updateOrientation(float dx, float dy)
  {
    // Rotation around the first axis
    vec3f strafevec = cross(up, view);
    const float c0 = cosf(dx);
    const float s0 = sinf(dx);
    const vec3f nextStrafe0 = normalize(c0 * strafevec + s0 * view);
    const vec3f nextView0 = normalize(-s0 * strafevec + c0 * view);
    const float acosAngle0 = abs(dot(nextView0, up));
    if (acosAngle0 < acosMinAngle) {
      view = nextView0;
      strafevec = nextStrafe0;
    }

    // Rotation around the second axis
    const float c1 = cosf(dy);
    const float s1 = sinf(dy);
    const vec3f nextView1 = normalize(s1 * up + c1 * view);
    const float acosAngle1 = abs(dot(nextView1, up));
    if (acosAngle1 < acosMinAngle) view = nextView1;
    lookAt = org + view;
  }

  void FPSCamera::updatePosition(const vec3f &d)
  {
    const vec3f strafe = cross(up, view);
    org += d.x * strafe;
    org += d.y * up;
    org += d.z * view;
  }

  TaskCamera::TaskCamera(FPSCamera &cam, InputControl &event) :
    Task("TaskCamera"), cam(&cam), event(&event) {}

  // use CVAR
  extern uint32 key_l;
  extern uint32 key_k;
  extern uint32 key_p;

  Task *TaskCamera::run(void)
  {
    // Change mouse position
    vec3f d(0.f);
    if (event->getKey('w')) d.z += float(event->dt) * cam->speed;
    if (event->getKey('s')) d.z -= float(event->dt) * cam->speed;
    if (event->getKey('a')) d.x += float(event->dt) * cam->speed;
    if (event->getKey('d')) d.x -= float(event->dt) * cam->speed;
    if (event->getKey('r')) d.y += float(event->dt) * cam->speed;
    if (event->getKey('f')) d.y -= float(event->dt) * cam->speed;
    key_l = event->getKey('l') ? 1 : 0;
    key_k = event->getKey('k') ? 1 : 0;
    key_p = event->getKey('p') ? 1 : 0;

    cam->updatePosition(d);
    cam->ratio = float(event->w) / float(event->h);

    // Change mouse orientation
    const float mouseXRel = float(event->mouseXRel);
    const float mouseYRel = float(event->mouseYRel);
    const float xrel = cam->angularSpeed * mouseXRel;
    const float yrel = cam->angularSpeed * mouseYRel;
    cam->updateOrientation(xrel, yrel);

    return NULL;
  }

} /* namespace pf */


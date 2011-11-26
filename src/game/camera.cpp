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

#include "renderer/hiz.hpp" // XXX to test the HiZ
#include "rt/bvh2.hpp"      // XXX to test the HiZ
#include "rt/bvh2_traverser.hpp"      // XXX to test the HiZ
#include "rt/rt_triangle.hpp"      // XXX to test the HiZ
#include "rt/rt_camera.hpp"      // XXX to test the HiZ
#include "image/stb_image.hpp"

namespace pf
{
  const float FPSCamera::defaultSpeed = 1.f;
  const float FPSCamera::defaultAngularSpeed = 4.f * 180.f / float(pi) / 50000.f;
  const float FPSCamera::acosMinAngle = 0.95f;

  FPSCamera::FPSCamera(const vec3f &org_,
                       const vec3f &up_,
                       const vec3f &view_,
                       float fov_,
                       float ratio_,
                       float near_,
                       float far_) :
    org(org_), up(up_), view(view_), lookAt(org_+view_),
    fov(fov_), ratio(ratio_), near(near_), far(far_),
    speed(defaultSpeed), angularSpeed(defaultAngularSpeed)
  {}

  FPSCamera::FPSCamera(const FPSCamera &other) {
    this->org = other.org;
    this->up = other.up;
    this->view = other.view;
    this->lookAt = other.lookAt;
    this->fov = other.fov;
    this->ratio = other.ratio;
    this->near = other.near;
    this->far = other.far;
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

  TaskCamera::TaskCamera(FPSCamera &cam, InputEvent &event) :
    Task("TaskCamera"), cam(&cam), event(&event) {}

  Ref<BVH2<RTTriangle>> bvh = NULL; // XXX -> HiZ

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

    // XXX for HiZ
    if (event->getKey('p')) {
      //Ref<HiZ> zbuffer = PF_NEW(HiZ, 1024, 1024);
      Ref<HiZ> zbuffer = PF_NEW(HiZ, 256, 128);
      //Ref<HiZ> zbuffer = PF_NEW(HiZ);
      Ref<Intersector> intersector = PF_NEW(BVH2Traverser<RTTriangle>, bvh);
      const RTCamera rt_cam(cam->org, cam->up, cam->view, cam->fov, cam->ratio);
      double t = getSeconds();
      Ref<Task> task = zbuffer->rayTrace(rt_cam, intersector);
      task->waitForCompletion();
      PF_MSG("HiZ ray tracing time " << getSeconds() - t);
      uint8 *rgba = PF_NEW_ARRAY(uint8, 4 * zbuffer->width * zbuffer->height);
      zbuffer->greyRGBA(&rgba);
      stbi_write_tga("hop.tga", zbuffer->width, zbuffer->height, 4, rgba);
      PF_DELETE_ARRAY(rgba);
    }
    if (event->getKey(27)) bvh = NULL;

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


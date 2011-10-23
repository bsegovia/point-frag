#include "camera.hpp"
#include "event.hpp"

namespace pf {

  const float FlyCamera::defaultSpeed = 1.f;
  const float FlyCamera::defaultAngularSpeed = 4.f * 180.f / float(pi) / 100.f;
  const float FlyCamera::acosMinAngle = 0.95f;

  FlyCamera::FlyCamera(const vec3f &pos_,
                       const vec3f &up_,
                       const vec3f &view_,
                       float fov_,
                       float ratio_,
                       float near_,
                       float far_) :
    pos(pos_), up(up_), view(view_), lookAt(up_+view_),
    fov(fov_), ratio(ratio_), near(near_), far(far_),
    speed(defaultSpeed), angularSpeed(defaultAngularSpeed)
  {}

  FlyCamera::FlyCamera(const FlyCamera &other) {
    this->pos = other.pos;
    this->up = other.up;
    this->view = other.view;
    this->fov = other.fov;
    this->ratio = other.ratio;
    this->near = other.near;
    this->far = other.far;
    this->speed = other.speed;
    this->angularSpeed = other.angularSpeed;
  }

  void FlyCamera::updateOrientation(float dx, float dy)
  {
    vec3f strafevec = cross(up, view);
    const float c0 = cosf(dx);
    const float s0 = sinf(dx);
    vec3f nextStrafe0 = c0 * strafevec + s0 * view;
    vec3f nextView0 = -s0 * strafevec + c0 * view;
    nextView0 = normalize(nextView0);
    nextStrafe0 = normalize(nextStrafe0);

    // Limit the angle with the up vector
    if (abs(dot(nextView0, up)) < acosMinAngle) {
      view = nextView0;
      strafevec = nextStrafe0;
    }

    const float c1 = cosf(dy);
    const float s1 = sinf(dy);
    vec3f nextView1 = s1 * up + c1 * view;
    nextView1 = normalize(nextView1);

    // Limit the angle with the up vector
    if (abs(dot(nextView1, up)) < acosMinAngle)
      view = nextView1;
    lookAt = pos + view;
  }

  void FlyCamera::updatePosition(const vec3f &d)
  {
    const vec3f strafe = cross(up, view);
    pos += d.x * strafe;
    pos += d.y * up;
    pos += d.z * view;
  }

  TaskCamera::TaskCamera(FlyCamera *cam, InputEvent *event) :
    Task("TaskCamera"), cam(cam), event(event) {}

  Task *TaskCamera::run(void)
  {
    // Change mouse orientation
    const float xrel = float(event->mouseXRel) * float(event->dt) * cam->angularSpeed;
    const float yrel = float(event->mouseYRel) * float(event->dt) * cam->angularSpeed;
    cam->updateOrientation(xrel, yrel);

    // Change mouse position
    vec3f d(0.f);
    if (event->getKey('w')) d.z += float(event->dt) * cam->speed;
    if (event->getKey('s')) d.z -= float(event->dt) * cam->speed;
    if (event->getKey('a')) d.x += float(event->dt) * cam->speed;
    if (event->getKey('d')) d.x -= float(event->dt) * cam->speed;
    if (event->getKey('r')) d.y += float(event->dt) * cam->speed;
    if (event->getKey('f')) d.y -= float(event->dt) * cam->speed;
    cam->updatePosition(d);
    cam->ratio = float(event->w) / float(event->h);

    return NULL;
  }
}


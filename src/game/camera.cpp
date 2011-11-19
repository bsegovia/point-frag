#include "camera.hpp"
#include "game_event.hpp"
#include "sys/logging.hpp"

#include "rt/bvh.hpp"
#include "rt/ray.hpp"
#include "rt/rt_camera.hpp"
#include "rt/rt_triangle.hpp"

#include "image/stb_image.hpp" // XXX to test the ray tracer

#include <cstring>
namespace pf {

  const float FlyCamera::defaultSpeed = 1.f;
  const float FlyCamera::defaultAngularSpeed = 4.f * 180.f / float(pi) / 50000.f;
  const float FlyCamera::acosMinAngle = 0.95f;

  FlyCamera::FlyCamera(const vec3f &pos_,
                       const vec3f &up_,
                       const vec3f &view_,
                       float fov_,
                       float ratio_,
                       float near_,
                       float far_) :
    pos(pos_), up(up_), view(view_), lookAt(pos_+view_),
    fov(fov_), ratio(ratio_), near(near_), far(far_),
    speed(defaultSpeed), angularSpeed(defaultAngularSpeed)
  {}

  FlyCamera::FlyCamera(const FlyCamera &other) {
    this->pos = other.pos;
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

  void FlyCamera::updateOrientation(float dx, float dy)
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
    lookAt = pos + view;
  }

  void FlyCamera::updatePosition(const vec3f &d)
  {
    const vec3f strafe = cross(up, view);
    pos += d.x * strafe;
    pos += d.y * up;
    pos += d.z * view;
  }

  TaskCamera::TaskCamera(FlyCamera &cam, InputEvent &event) :
    Task("TaskCamera"), cam(&cam), event(&event) {}

  /* XXX to test the ray tracing */
  extern Ref<BVH2<RTTriangle>> bvh;
  template <typename T> void trace(const BVH2<T> &bvh, const Ray &ray, Hit &hit);
  static const int CAMW = 1024, CAMH = 1024;

  class TaskRayTrace : public TaskSet
  {
  public:
    INLINE TaskRayTrace(const BVH2<RTTriangle> &bvh,
                        const RTCameraRayGen &gen,
                        const uint32 *c,
                        uint32 *rgba,
                        uint32 w, uint32 h) :
      TaskSet(h, "TaskRayTrace"), bvh(bvh), gen(gen), c(c), rgba(rgba), w(w) {}
    virtual void run(size_t y) {
      for (uint32 x = 0; x < w; ++x) {
        Ray ray;
        Hit hit;
        gen.generate(ray, x, y);
        trace(bvh, ray, hit);
        rgba[x + y*w] = hit ? c[hit.id0] : 0u;
      }
    }
    const BVH2<RTTriangle> &bvh;
    const RTCameraRayGen &gen;
    const uint32 *c;
    uint32 *rgba;
    uint32 w;
  };

  static void rayTrace(const FlyCamera &flyCam, int w, int h)
  {
    const RTCamera cam(flyCam.pos, flyCam.lookAt, flyCam.up, flyCam.fov, flyCam.ratio);
    RTCameraRayGen gen;
    cam.createGenerator(gen, w, h);
    uint32 *rgba = PF_NEW_ARRAY(uint32, w * h);
    uint32 *c = PF_NEW_ARRAY(uint32, bvh->primNum);
    std::memset(rgba, 0, sizeof(uint32) * w * h);
    for (uint32 i = 0; i < bvh->primNum; ++i) {
      ((uint8*)&c[i])[0] = (rand() >> 16) & 0xff;
      ((uint8*)&c[i])[1] = 0xff;
      ((uint8*)&c[i])[2] = (rand() >> 16) & 0xff;
      ((uint8*)&c[i])[3] = (rand() >> 16) & 0xff;
    }
    const double t = getSeconds();
    Ref<Task> rayTask = PF_NEW(TaskRayTrace, *bvh, gen, c, rgba, w, h);
    rayTask->scheduled();
    rayTask->waitForCompletion();
    const double dt = getSeconds() - t;
    PF_MSG_V("ray tracing time: " << dt);
    stbi_write_bmp("hop.bmp", w, h, 4, rgba);
    PF_DELETE_ARRAY(rgba);
    PF_DELETE_ARRAY(c);
  }

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

    if (event->getKey('p')) rayTrace(*cam, CAMW, CAMH); // XXX
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
}


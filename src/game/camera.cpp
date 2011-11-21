#include "camera.hpp"
#include "game_event.hpp"
#include "sys/logging.hpp"

#include "rt/bvh.hpp"           // XXX to test the ray tracer
#include "rt/ray.hpp"           // XXX to test the ray tracer
#include "rt/rt_camera.hpp"     // XXX to test the ray tracer
#include "rt/rt_triangle.hpp"   // XXX to test the ray tracer
#include "image/stb_image.hpp"  // XXX to test the ray tracer

#include <cstring>
namespace pf {

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

  /* XXX to test the ray tracing */
  extern Ref<BVH2<RTTriangle>> bvh;
  static const int CAMW = 1024, CAMH = 1024;

  class TaskRayTrace : public TaskSet
  {
  public:
    INLINE TaskRayTrace(const BVH2<RTTriangle> &bvh,
                        const RTCamera &cam,
                        const uint32 *c,
                        uint32 *rgba,
                        uint32 w, uint32 jobNum) :
      TaskSet(jobNum, "TaskRayTrace"),
      bvh(bvh), cam(cam), c(c), rgba(rgba), w(w), h(jobNum * RayPacket::height) {}
#if 0
    virtual void run(size_t jobID)
    {
      RTCameraRayGen gen;
      cam.createGenerator(gen, w, h);
      for (uint32 row = 0; row < RayPacket::height; ++row) {
        const uint32 y = row + jobID * RayPacket::height;
        for (uint32 x = 0; x < w; ++x) {
          Ray ray;
          Hit hit;
          gen.generate(ray, x, y);
          bvh.traverse(ray, hit);
          rgba[x + y*w] = hit ? c[hit.id0] : 0u;
        }
      }
    }
#else
#define USE_MORTON 0
    virtual void run(size_t jobID)
    {
      RTCameraPacketGen gen;
      cam.createGenerator(gen, w, h);
      const uint32 y = jobID * RayPacket::height;
      for (uint32 x = 0; x < w; x += RayPacket::width) {
        RayPacket pckt;
        PacketHit hit;
#if USE_MORTON
        gen.generateMorton(pckt, x, y);
#else
        gen.generate(pckt, x, y);
#endif
        bvh.traverse(pckt, hit);
        const int32 *IDs = (const int32 *) hit.id0;
        uint32 curr = 0;
        for (uint32 j = 0; j < pckt.height; ++j)
        for (uint32 i = 0; i < pckt.width; ++i, ++curr) {
#if USE_MORTON
          const uint32 i0 = RTCameraPacketGen::mortonX[curr];
          const uint32 j0 = RTCameraPacketGen::mortonY[curr];
#else
          const uint32 i0 = i;
          const uint32 j0 = j;
#endif
          const uint32 offset = x + i0 + (y + j0) * w;
          rgba[offset] = IDs[curr] != -1 ? c[IDs[curr]] : 0u;
        }
      }
    }

#endif
    const BVH2<RTTriangle> &bvh;
    const RTCamera &cam;
    const uint32 *c;
    uint32 *rgba;
    uint32 w, h;
  };

  static void rayTrace(const FPSCamera &fpsCam, int w, int h)
  {
    const RTCamera cam(fpsCam.org, fpsCam.up, fpsCam.view, fpsCam.fov, fpsCam.ratio);
    uint32 *rgba = PF_NEW_ARRAY(uint32, w * h);
    uint32 *c = PF_NEW_ARRAY(uint32, bvh->primNum);
    std::memset(rgba, 0, sizeof(uint32) * w * h);
    for (uint32 i = 0; i < bvh->primNum; ++i) {
      c[i] = rand();
      c[i] |= 0xff000000;
    }
    const double t = getSeconds();
    Ref<Task> rayTask = PF_NEW(TaskRayTrace, *bvh, cam, c, rgba, w, h/RayPacket::height);
    rayTask->scheduled();
    rayTask->waitForCompletion();
    const double dt = getSeconds() - t;
    PF_MSG_V("ray tracing time: " << dt);
    stbi_write_tga("hop.tga", w, h, 4, rgba);
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


#include "camera.hpp"

#include <cmath>

using namespace pf;

FlyCamera::FlyCamera(const vec3f &pos_,
                     const vec3f &up_,
                     const vec3f &view_,
                     float fov_,
                     float ratio_,
                     float near_,
                     float far_) :
  pos(pos_), up(up_), view(view_), lookAt(up_+view_), fov(fov_), ratio(ratio_),
  near(near_), far(far_)
{}

void FlyCamera::updateOrientation(float dx, float dy)
{
  vec3f strafevec = cross(up, view);
  const float c0 = cosf(dx);
  const float s0 = sinf(dx);
  const vec3f nextStrafe0 = c0 * strafevec + s0 * view;
  const vec3f nextView0 = -s0 * strafevec + c0 * view;
  view = nextView0;
  strafevec = nextStrafe0;

  const float c1 = cosf(dy);
  const float s1 = sinf(dy);
  const vec3f nextView1 = s1 * up + c1 * view;
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


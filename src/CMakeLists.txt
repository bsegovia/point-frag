set (SRC
  app/camera.cpp
  app/app.cpp
  sys/filename.cpp
  sys/library.cpp
  sys/thread.cpp
  sys/tasking.cpp
  sys/sysinfo.cpp
  sys/mutex.cpp
  sys/condition.cpp
  sys/platform.cpp
  sys/string.cpp
  image/stb_image.cpp
  rtcore/rtcore.cpp
  rtcore/common/compute_bounds.cpp
  rtcore/common/object_binning.cpp
  rtcore/common/object_binning_parallel.cpp
  rtcore/common/spatial_binning.cpp
  rtcore/bvh2/bvh2.cpp
  rtcore/bvh2/bvh2_builder.cpp
  rtcore/bvh2/bvh2_builder_spatial.cpp
  rtcore/bvh2/bvh2_to_bvh4.cpp
  rtcore/bvh2/bvh2_traverser.cpp
  rtcore/bvh4/bvh4.cpp
  rtcore/bvh4/bvh4_builder.cpp
  rtcore/bvh4/bvh4_traverser.cpp
  math/distribution.cpp
  models/obj.cpp
  renderer/ogl.cpp
  renderer/texture.cpp
  renderer/renderobj.cpp
  renderer/renderer.cpp)
include_directories (.)
add_executable (app ${SRC})
target_link_libraries(app glut pthread)


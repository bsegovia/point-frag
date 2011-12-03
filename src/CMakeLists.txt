include (FindGLUT)
include_directories (${GLUT_INCLUDE_PATH})
set (SRC
  game/camera.cpp
  game/game_event.cpp
  game/game_frame.cpp
  game/game_render.cpp
  sys/filename.cpp
  sys/library.cpp
  sys/thread.cpp
  sys/alloc.cpp
  sys/tasking.cpp
  sys/sysinfo.cpp
  sys/mutex.cpp
  sys/condition.cpp
  sys/platform.cpp
  sys/string.cpp
  sys/logging.cpp
  sys/default_path.cpp
  stl/allocator.cpp
  stl/intrusive_list.cpp
  stl/intrusive_slist.cpp
  stl/list.cpp
  stl/slist.cpp
  stl/test.cpp
  simd/ssef.cpp
  image/stb_image.cpp
  image/stb_image_write.cpp
  image/squish/alpha.cpp
  image/squish/clusterfit.cpp
  image/squish/colourblock.cpp
  image/squish/colourfit.cpp
  image/squish/colourset.cpp
  image/squish/maths.cpp
  image/squish/rangefit.cpp
  image/squish/singlecolourfit.cpp
  image/squish/squish.cpp
  models/obj.cpp
  renderer/ogl.cpp
  renderer/hiz.cpp
  renderer/texture.cpp
  renderer/renderer.cpp
  renderer/renderer_obj.cpp
  renderer/renderer_driver.cpp
  rt/bvh2.cpp
  rt/bvh2_traverser.cpp
  rt/ray_packet.cpp
  rt/rt_camera.cpp)
include_directories (.)
add_library (pf ${SRC})
if (UNIX)
  target_link_libraries(pf ${GLUT_LIBRARY} pthread)
  #target_link_libraries(pf pthread GL X11 Xrandr Xext Xxf86vm Xi)
else (UNIX)
  target_link_libraries(pf ${GLUT_LIBRARY} opengl32)
  #target_link_libraries(pf opengl32 winmm)
endif (UNIX)

add_executable (game game/game.cpp)
target_link_libraries (game pf)

add_executable (rt test/rt.cpp)
target_link_libraries (rt pf)

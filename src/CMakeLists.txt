include (FindGLUT)
include_directories (${GLUT_INCLUDE_PATH})
set (SRC
  game/camera.cpp
  game/game.cpp
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
  image/stb_image.cpp
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
  renderer/texture.cpp
  renderer/renderer.cpp
  renderer/renderer_obj.cpp
  renderer/renderer_driver.cpp
  rt/bvh.cpp)
include_directories (.)
add_executable (app ${SRC})
if (UNIX)
  target_link_libraries(app ${GLUT_LIBRARY} pthread)
else (UNIX)
  target_link_libraries(app ${GLUT_LIBRARY})
endif (UNIX)


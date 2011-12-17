include (FindGLUT)
include (FindLuaJIT)
include_directories (${GLUT_INCLUDE_PATH})
set (SRC
  game/main.cpp
  game/game.cpp
  game/camera.cpp
  game/game_event.cpp
  game/game_frame.cpp
  game/game_render.cpp
  sys/filename.cpp
  sys/library.cpp
  sys/thread.cpp
  sys/alloc.cpp
  sys/tasking.cpp
  sys/tasking_utility.cpp
  sys/sysinfo.cpp
  sys/mutex.cpp
  sys/condition.cpp
  sys/platform.cpp
  sys/string.cpp
  sys/logging.cpp
  sys/default_path.cpp
  sys/script.cpp
  sys/command.cpp
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
  renderer/font.cpp
  renderer/hiz.cpp
  renderer/texture.cpp
  renderer/renderer.cpp
  renderer/renderer_obj.cpp
  renderer/renderer_driver.cpp
  renderer/renderer_context.cpp
  rt/bvh2.cpp
  rt/bvh2_traverser.cpp
  rt/ray_packet.cpp
  rt/rt_camera.cpp
  utest/utest.cpp)

set (COMPILE_UTEST false CACHE bool "Compile or not the unit tests")
if (COMPILE_UTEST)
  set (SRC ${SRC}
       utest/utest_lua.cpp
       utest/utest_rt.cpp
       utest/utest_font.cpp
       utest/utest_tasking.cpp)
endif (COMPILE_UTEST)

include_directories (.)
include_directories (${LUAJIT_INCLUDE_DIR})

if (UNIX)
  set (EXEC_DEPENDENCIES ${GLUT_LIBRARY} ${LUAJIT_LIBRARY} pthread)
else (UNIX)
  set (EXEC_DEPENDENCIES ${GLUT_LIBRARY} ${LUAJIT_LIBRARY} OpenGL32)
endif (UNIX)

add_executable (game ${SRC})
target_link_libraries (game ${EXEC_DEPENDENCIES})


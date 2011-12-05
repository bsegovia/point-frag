include (FindGLUT)
include (FindLuaJIT)
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
  rt/bvh2.cpp
  rt/bvh2_traverser.cpp
  rt/ray_packet.cpp
  rt/rt_camera.cpp)
include_directories (.)
include_directories (${LUAJIT_INCLUDE_DIR})

if (UNIX)
  set (EXEC_DEPENDENCIES ${GLUT_LIBRARY} ${LUAJIT_LIBRARY} pthread)
else (UNIX)
  set (EXEC_DEPENDENCIES ${GLUT_LIBRARY} ${LUAJIT_LIBRARY})
endif (UNIX)

# dirty but otherwise, static variables are optimized out while using static
# libraries. Dynamic libraries work but the code does not support it right now
set (COMPILE_TEST_LUA false CACHE bool "Compile or not test_lua")
if (COMPILE_TEST_LUA)
  add_executable (test_lua ${SRC} test/lua.cpp)
  target_link_libraries (test_lua ${EXEC_DEPENDENCIES})
endif ()

set (COMPILE_TEST_RT false CACHE bool "Compile or not test_rt")
if (COMPILE_TEST_RT)
  add_executable (test_rt ${SRC} test/rt.cpp)
  target_link_libraries (test_rt ${EXEC_DEPENDENCIES})
endif ()

set (COMPILE_TEST_FONT false CACHE bool "Compile or not test_rt")
if (COMPILE_TEST_FONT)
  add_executable (test_font ${SRC} test/font.cpp)
  target_link_libraries (test_font ${EXEC_DEPENDENCIES})
endif ()

set (COMPILE_GAME true CACHE bool "Compile or not game")
if (COMPILE_GAME)
  add_executable (game ${SRC} game/game.cpp)
  target_link_libraries (game ${EXEC_DEPENDENCIES})
endif ()


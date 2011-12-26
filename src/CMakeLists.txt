include (FindGLUT)
include (FindLuaJIT)
include_directories (${GLUT_INCLUDE_PATH})
set (SRC
  game/main.cpp
  game/game.cpp
  game/game.hpp
  game/camera.cpp
  game/camera.hpp
  game/game_event.cpp
  game/game_event.hpp
  game/game_frame.cpp
  game/game_frame.hpp
  game/game_render.cpp
  game/game_render.hpp
  sys/array.hpp
  sys/vector.hpp
  sys/filename.cpp
  sys/filename.hpp
  sys/library.cpp
  sys/library.hpp
  sys/thread.cpp
  sys/thread.hpp
  sys/alloc.cpp
  sys/alloc.hpp
  sys/tasking.cpp
  sys/tasking.hpp
  sys/tasking_utility.cpp
  sys/tasking_utility.hpp
  sys/sysinfo.cpp
  sys/sysinfo.hpp
  sys/ref.hpp
  sys/mutex.cpp
  sys/mutex.hpp
  sys/condition.cpp
  sys/condition.hpp
  sys/platform.cpp
  sys/platform.hpp
  sys/string.cpp
  sys/string.hpp
  sys/logging.cpp
  sys/logging.hpp
  sys/default_path.cpp
  sys/default_path.hpp
  sys/script.cpp
  sys/script.hpp
  sys/command.cpp
  sys/command.hpp
  simd/ssef.cpp
  image/stb_image.cpp
  image/stb_image.hpp
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
  models/obj.hpp
  renderer/ogl.cpp
  renderer/ogl.hpp
  renderer/font.cpp
  renderer/font.hpp
  renderer/hiz.cpp
  renderer/hiz.hpp
  renderer/texture.cpp
  renderer/texture.hpp
  renderer/renderer.cpp
  renderer/renderer.hpp
  renderer/renderer_object.cpp
  renderer/renderer_object.hpp
  renderer/renderer_obj.cpp
  renderer/renderer_obj.hpp
  renderer/renderer_set.cpp
  renderer/renderer_set.hpp
  renderer/renderer_displayable.cpp
  renderer/renderer_displayable.hpp
  renderer/renderer_display_list.cpp
  renderer/renderer_display_list.hpp
  renderer/renderer_driver.cpp
  renderer/renderer_driver.hpp
  renderer/renderer_frame.cpp
  renderer/renderer_frame.hpp
  renderer/renderer_context.cpp
  renderer/renderer_context.hpp
  rt/intersector.hpp
  rt/bvh2.cpp
  rt/bvh2.hpp
  rt/bvh2_traverser.cpp
  rt/bvh2_traverser.hpp
  rt/ray_packet.cpp
  rt/ray_packet.hpp
  rt/rt_camera.cpp
  rt/rt_camera.hpp
  utest/utest.cpp
  utest/utest.hpp)

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


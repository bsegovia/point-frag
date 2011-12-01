include (FindGLUT)
include_directories (${GLUT_INCLUDE_PATH})
set (SRC

freeglut/freeglut_internal.h
freeglut/freeglut_teapot_data.h
freeglut/freeglut_callbacks.c
freeglut/freeglut_cursor.c
freeglut/freeglut_display.c
freeglut/freeglut_ext.c
freeglut/freeglut_font.c
freeglut/freeglut_font_data.c
freeglut/freeglut_gamemode.c
freeglut/freeglut_geometry.c
freeglut/freeglut_glutfont_definitions.c
freeglut/freeglut_init.c
freeglut/freeglut_joystick.c
freeglut/freeglut_main.c
freeglut/freeglut_menu.c
freeglut/freeglut_misc.c
freeglut/freeglut_overlay.c
freeglut/freeglut_spaceball.c
freeglut/freeglut_state.c
freeglut/freeglut_stroke_mono_roman.c
freeglut/freeglut_stroke_roman.c
freeglut/freeglut_structure.c
freeglut/freeglut_teapot.c
freeglut/freeglut_videoresize.c
freeglut/freeglut_window.c
freeglut/freeglut_xinput.c
freeglut/freeglut_input_devices.c
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
#target_link_libraries(pf ${GLUT_LIBRARY} pthread)
  target_link_libraries(pf pthread GL X11 Xrandr Xext Xxf86vm Xi)
else (UNIX)
#  target_link_libraries(pf ${GLUT_LIBRARY} opengl32)
 target_link_libraries(pf opengl32)
endif (UNIX)

add_executable (game game/game.cpp)
target_link_libraries (game pf)

add_executable (rt test/rt.cpp)
target_link_libraries (rt pf)

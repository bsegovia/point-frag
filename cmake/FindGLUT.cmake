# - try to find glut library and include files
#  GLUT_INCLUDE_DIR, where to find GL/glut.h, etc.
#  GLUT_LIBRARIES, the libraries to link against
#  GLUT_FOUND, If false, do not try to use GLUT.
# Also defined, but not for general use are:
#  GLUT_glut_LIBRARY = the full path to the glut library.
#  GLUT_Xmu_LIBRARY  = the full path to the Xmu library.
#  GLUT_Xi_LIBRARY   = the full path to the Xi Library.

if (WIN32)
  find_path (GLUT_INCLUDE_DIR GL/glut.h
             ${GLUT_ROOT_PATH}/include
             ${PROJECT_SOURCE_DIR}/extern/glut/include
             doc "The directory where GL/glut.h resides")
  find_library (GLUT_glut_LIBRARY
                NAMES freeglut glut GLUT glut32 glut32s
                PATHS
                ${GLUT_ROOT_PATH}/lib
                ${PROJECT_SOURCE_DIR}/extern/glut/lib
                ${OPENGL_LIBRARY_DIR}
                DOC "The GLUT library")
else (WIN32)
  if (APPLE)
    find_path (GLUT_INCLUDE_DIR glut.h
               /System/Library/Frameworks/GLUT.framework/Versions/A/Headers
               ${OPENGL_LIBRARY_DIR})
    set (GLUT_glut_LIBRARY "-framework Glut" CACHE STRING "GLUT library for OSX") 
    set (GLUT_cocoa_LIBRARY "-framework Cocoa" CACHE STRING "Cocoa framework for OSX")
  else (APPLE)
    find_path (GLUT_INCLUDE_DIR GL/glut.h
               /usr/include
               /usr/include/GL
               /usr/local/include
               /usr/openwin/share/include
               /usr/openwin/include
               /usr/X11R6/include
               /usr/include/X11
               /opt/graphics/OpenGL/include
               /opt/graphics/OpenGL/contrib/libglut)
    find_library (GLUT_glut_LIBRARY glut
                  /usr/lib
                  /usr/local/lib
                  /usr/openwin/lib
                  /usr/X11R6/lib)
    find_library (GLUT_Xi_LIBRARY Xi
                  /usr/lib
                  /usr/local/lib
                  /usr/openwin/lib
                  /usr/X11R6/lib)
    find_library (GLUT_Xmu_LIBRARY Xmu
                  /usr/lib
                  /usr/local/lib
                  /usr/openwin/lib
                  /usr/X11R6/lib)
  endif (APPLE)
endif (WIN32)

set (GLUT_FOUND "NO")
if (GLUT_INCLUDE_DIR)
  if (GLUT_glut_LIBRARY)
    set (GLUT_LIBRARIES
         ${GLUT_glut_LIBRARY}
         ${GLUT_Xmu_LIBRARY}
         ${GLUT_Xi_LIBRARY} 
         ${GLUT_cocoa_LIBRARY})
    set (GLUT_FOUND "YES")
    set (GLUT_LIBRARY ${GLUT_LIBRARIES})
    set (GLUT_INCLUDE_PATH ${GLUT_INCLUDE_DIR})
  endif(GLUT_glut_LIBRARY)
endif(GLUT_INCLUDE_DIR)

mark_as_advanced (GLUT_INCLUDE_DIR
                  GLUT_glut_LIBRARY
                  GLUT_Xmu_LIBRARY
                  GLUT_Xi_LIBRARY)


# LUAJIT_FOUND
# LUAJIT_INCLUDE_DIR
# LUAJIT_LIBRARY

if (WIN32)
  find_path (LUAJIT_INCLUDE_DIR lua.hpp
             ${LUAJIT_ROOT_PATH}/include
             ${PROJECT_SOURCE_DIR}/extern/luajit/include
             doc "The directory where lua.hpp and other lua headers reside")
  find_library (LUAJIT_LIBRARY
                NAMES luajit-5.1
                PATHS
                ${PROJECT_SOURCE_DIR}/extern/luajit/lib
                DOC "The LuaJIT library")
else (WIN32)
    find_path (LUAJIT_INCLUDE_DIR lua.hpp
               ~/include/
               /usr/include
               /usr/local/include)
    find_library (LUAJIT_LIBRARY luajit-5.1
                  ~/lib/
                  /usr/lib
                  /usr/local/lib)
endif (WIN32)

if (LUAJIT_INCLUDE_DIR)
  set (LUAJIT_FOUND 1 CACHE STRING "Set to 1 if LUAJIT is found, 0 otherwise")
else (LUAJIT_INCLUDE_DIR)
  set (LUAJIT_FOUND 0 CACHE STRING "Set to 1 if LUAJIT is found, 0 otherwise")
endif (LUAJIT_INCLUDE_DIR)

mark_as_advanced (LUAJIT_FOUND)


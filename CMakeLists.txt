cmake_minimum_required (VERSION 2.6.0)

project (PF)
set (PF_CMAKE_DIR "${PF_SOURCE_DIR}/cmake")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PF_CMAKE_DIR}")
set (PF_DEBUG_MEMORY CACHE bool false)
set (PF_USE_BLOB CACHE bool false)

if (UNIX)
  set (DEF "-D")
else (UNIX)
  set (DEF "/D")
endif (UNIX)

if (PF_DEBUG_MEMORY)
  set (PF_DEBUG_MEMORY_FLAG "${DEF}PF_DEBUG_MEMORY=1")
else (PF_DEBUG_MEMORY)
  set (PF_DEBUG_MEMORY_FLAG "${DEF}PF_DEBUG_MEMORY=0")
endif (PF_DEBUG_MEMORY)

if (UNIX)
  set (COMMON_FLAGS "${PF_DEBUG_MEMORY_FLAG}")
  set (CMAKE_CXX_FLAGS "${COMMON_FLAGS} -msse2 -msse3 -mssse3 -fno-rtti -fno-exceptions -Wall -std=c++0x")
  set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
  set (CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
  set (CMAKE_C_FLAGS "${COMMON_FLAGS} -msse2 -msse3 -mssse3 -Wall")
else (UNIX)
  set (COMMON_FLAGS "${PF_DEBUG_MEMORY_FLAG} /DDDDDD=1 /D_CRT_SECURE_NO_WARNINGS /D_HAS_EXCEPTIONS=0 /GR- /W3 /wd4275")
  set (CMAKE_CXX_FLAGS ${COMMON_FLAGS})
  set (CMAKE_C_FLAGS ${COMMON_FLAGS})
endif (UNIX)

add_subdirectory (src)


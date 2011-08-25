cmake_minimum_required (VERSION 2.6.0)

project (PF)
set (PF_CMAKE_DIR "${PF_SOURCE_DIR}/cmake")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PF_CMAKE_DIR}")

if (UNIX)
  set (CMAKE_CXX_FLAGS "-msse2 -msse3 -mssse3 -Wall -std=c++0x")
  set (CMAKE_C_FLAGS "-msse2 -msse3 -mssse3 -Wall")
elseif (WIN32)
  set (CMAKE_CXX_FLAGS "/D_CRT_SECURE_NO_WARNINGS")
  set (CMAKE_C_FLAGS "/D_CRT_SECURE_NO_WARNINGS")
endif (UNIX)
add_subdirectory (src)


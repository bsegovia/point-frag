cmake_minimum_required (VERSION 2.6.0)
set (CMAKE_CXX_FLAGS "-msse2 -msse3 -mssse3 -Wall -std=c++0x")
set (CMAKE_C_FLAGS "-msse2 -msse3 -mssse3 -Wall")
add_subdirectory (src)


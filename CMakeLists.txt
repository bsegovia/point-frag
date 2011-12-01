project (PF)

cmake_minimum_required (VERSION 2.6.0)

set (PF_CMAKE_DIR "${PF_SOURCE_DIR}/cmake")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PF_CMAKE_DIR}")

##############################################################
# Compilation directives
##############################################################

set (PF_DEBUG_MEMORY false CACHE bool "Activate the memory debugger")
set (PF_USE_BLOB false CACHE bool "Compile everything from one big file")
set (PF_VERBOSE_VECTORIZER false CACHE bool "Output vectorizer diagnostic (GCC only)")

##############################################################
# Compiler
##############################################################
if (UNIX)
  set (COMPILER "GCC" CACHE INT "Compiler to choose on Linux (GCC,ICC)")
endif (UNIX)

if (WIN32)
  set (DEF "/D")
else (WIN32)
  set (DEF "-D")
endif (WIN32)

if (PF_DEBUG_MEMORY)
  set (PF_DEBUG_MEMORY_FLAG "${DEF}PF_DEBUG_MEMORY=1")
else (PF_DEBUG_MEMORY)
  set (PF_DEBUG_MEMORY_FLAG "${DEF}PF_DEBUG_MEMORY=0")
endif (PF_DEBUG_MEMORY)

## Linux compilation
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if (COMPILER STREQUAL "GCC")
    if (PF_VERBOSE_VECTORIZER)
      set (CMAKE_CXX_FLAGS "-ftree-vectorizer-verbose=2")
    endif (PF_VERBOSE_VECTORIZER)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PF_DEBUG_MEMORY_FLAG} -fstrict-aliasing -msse2 -ffast-math -fPIC -Wall -fno-rtti -fno-exceptions -std=c++0x")
    set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -ftree-vectorize")
  elseif (COMPILER STREQUAL "ICC")
    set (CMAKE_CXX_COMPILER "icpc")
    set (CMAKE_C_COMPILER "icc")
    set (CMAKE_CXX_FLAGS "${PF_DEBUG_MEMORY_FLAG} -std=c++0x -wd2928 -Wall -fPIC -fstrict-aliasing -fp-model fast -xSSE2")
    set (CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2")
    set (CCMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")
    set (CCMAKE_CXX_FLAGS_MINSIZEREL "-Os")
    set (CMAKE_EXE_LINKER_FLAGS "")
  endif ()

## Windows compilation
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  if (MINGW)
     if (PF_VERBOSE_VECTORIZER)
      set (CMAKE_CXX_FLAGS "-ftree-vectorizer-verbose=2")
    endif (PF_VERBOSE_VECTORIZER)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PF_DEBUG_MEMORY_FLAG} -fstrict-aliasing -msse2 -ffast-math -fPIC -Wall -fno-rtti -fno-exceptions -std=c++0x")
    set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -ftree-vectorize")
  else (MINGW)
    set (COMMON_FLAGS "${PF_DEBUG_MEMORY_FLAG} /arch:SSE2 /D_CRT_SECURE_NO_WARNINGS /D_HAS_EXCEPTIONS=0 /DNOMINMAX /GR- /GS- /W3 /wd4275")
    set (CMAKE_CXX_FLAGS ${COMMON_FLAGS})
    set (CMAKE_C_FLAGS ${COMMON_FLAGS})
  endif (MINGW)
endif ()

##############################################################
# Project source code
##############################################################
add_subdirectory (src)

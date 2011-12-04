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
  set (COMPILER "GCC" CACHE INT "Compiler to choose on Linux (GCC,ICC,CLANG)")
endif (UNIX)

if (WIN32)
  if (MINGW)
    set (DEF "-D")
  else (MINGW)
    set (DEF "/D")
  endif (MINGW)
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
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PF_DEBUG_MEMORY_FLAG} -Wstrict-aliasing=2 -fstrict-aliasing -msse2 -ffast-math -fPIC -Wall -fno-rtti -fno-exceptions -std=c++0x")
    set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -ftree-vectorize")
  elseif (COMPILER STREQUAL "CLANG")
    SET (CMAKE_C_COMPILER             "clang")
    SET (CMAKE_C_FLAGS                "-Wall -std=c99")
    SET (CMAKE_C_FLAGS_DEBUG          "-g")
    SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELEASE        "-O3 -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
    SET (CMAKE_CXX_COMPILER             "clang++")
    SET (CMAKE_CXX_FLAGS                "-Wall")
    SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
    SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
    SET (CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
    SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
    SET (CMAKE_AR      "/usr/bin/llvm-ar")
    SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
    SET (CMAKE_NM      "/usr/bin/llvm-nm")
    SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
    SET (CMAKE_RANLIB  "ranlib")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PF_DEBUG_MEMORY_FLAG} -fstrict-aliasing -msse2 -ffast-math -fPIC -Wall -Wno-format-security -Wno-invalid-offsetof -std=c++0x")
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
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PF_DEBUG_MEMORY_FLAG} -fstrict-aliasing -msse2 -ffast-math -Wall -fno-rtti -fno-exceptions -std=c++0x")
    set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
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

point-frag
==========

point-frag is a small code base to play with multi-threading and distributed
tasking systems for a game engine. The overall idea is to remove the notion of
"main" thread or "rendering" thread and to only have worker threads. The game
itself is just a infinite task that respawns itself all the time from frame to
frame.

How to build
------------

The code base uses CMake to build. CMake can be downloaded at www.cmake.org
The classical CMake profiles are supported (Debug, Release, RelWithDebInfo,
MinSizeRel).

- On Windows, use basically the user interface to generate the VS solutions.
- On Linux, you can use ccmake to get the similar user interface.
- The code may work on MacOS but I did not have a Mac to test it.

The code was tested and compiled on:
- Linux 32/64 bits. You need a relatively new compiler since the code use
  unordered\_map. Note that the code can compile with both GCC and clang. ICC
  is not supported due to some problems related to the use of some c++11
  features
- Windows 32 bits with VS2008 (but memory debugger which uses unordered\_map is
  not supported)
- Windows 32/64 bits with VS2010

Beyond the build mode, you may choose to use a memory debugger (it will slow
down the system considerably since it locks malloc/free) by setting the
variable PF\_DEBUG\_MEMORY with CMake

Also, note that parts of the code (related to mutexes/conditions/multi-platform
abstraction) were directly taken from Intel Embree project:
http://software.intel.com/en-us/articles/embree-photo-realistic-ray-tracing-kernels/

How to run
----------

Just run game (or game.exe). The code has been tested on recent AMD and NVidia
cards

Contact
-------

Benjamin Segovia (segovia.benjamin@gmail.com)


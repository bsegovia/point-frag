cmake_minimum_required (VERSION 2.6.0)

project (PF)
set (PF_CMAKE_DIR "${PF_SOURCE_DIR}/cmake")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PF_CMAKE_DIR}")
set (PF_DEBUG_MEMORY false CACHE bool "Activate the memory debugger")
set (PF_USE_BLOB false CACHE bool "Compile everything from one big file")
set (PF_VERBOSE_VECTORIZER false CACHE bool "Output vectorizer diagnostic (GCC only)")

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
  if (PF_VERBOSE_VECTORIZER)
    set (COMMON_FLAGS "-ftree-vectorizer-verbose=2")
  endif (PF_VERBOSE_VECTORIZER)
  set (COMMON_FLAGS "${COMMON_FLAGS} ${PF_DEBUG_MEMORY_FLAG}")
  set (CMAKE_CXX_FLAGS "${COMMON_FLAGS} -msse2 -fno-rtti -fno-exceptions -Wall -std=c++0x")
  set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -ftree-vectorize -ftree-vectorizer-verbose=2")
  set (CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -ftree-vectorize -ftree-vectorizer-verbose=2")
  set (CMAKE_C_FLAGS "${COMMON_FLAGS} -msse2 -Wall")
else (UNIX)
  set (COMMON_FLAGS "${PF_DEBUG_MEMORY_FLAG} /D_CRT_SECURE_NO_WARNINGS /D_HAS_EXCEPTIONS=0 /GR- /W3 /wd4275")
  set (CMAKE_CXX_FLAGS ${COMMON_FLAGS})
  set (CMAKE_C_FLAGS ${COMMON_FLAGS})
endif (UNIX)

include_directories (include/ ode/src)
set (SRC
drawstuff/src/drawstuff.cpp
drawstuff/src/x11.cpp
OPCODE/OPC_Picking.cpp
OPCODE/OPC_RayCollider.cpp
OPCODE/OPC_OptimizedTree.cpp
OPCODE/OPC_MeshInterface.cpp
OPCODE/Opcode.cpp
OPCODE/OPC_PlanesCollider.cpp
OPCODE/OPC_VolumeCollider.cpp
OPCODE/OPC_Common.cpp
OPCODE/OPC_LSSCollider.cpp
OPCODE/OPC_SphereCollider.cpp
OPCODE/OPC_TreeBuilders.cpp
OPCODE/OPC_OBBCollider.cpp
OPCODE/OPC_TreeCollider.cpp
OPCODE/OPC_AABBTree.cpp
OPCODE/OPC_BaseModel.cpp
OPCODE/Ice/IceIndexedTriangle.cpp
OPCODE/Ice/IceMatrix4x4.cpp
OPCODE/Ice/IcePlane.cpp
OPCODE/Ice/IceSegment.cpp
OPCODE/Ice/IcePoint.cpp
OPCODE/Ice/IceOBB.cpp
OPCODE/Ice/IceUtils.cpp
OPCODE/Ice/IceContainer.cpp
OPCODE/Ice/IceHPoint.cpp
OPCODE/Ice/IceMatrix3x3.cpp
OPCODE/Ice/IceRandom.cpp
OPCODE/Ice/IceAABB.cpp
OPCODE/Ice/IceRevisitedRadix.cpp
OPCODE/Ice/IceRay.cpp
OPCODE/Ice/IceTriangle.cpp
OPCODE/OPC_AABBCollider.cpp
OPCODE/OPC_HybridModel.cpp
OPCODE/OPC_Model.cpp
OPCODE/OPC_Collider.cpp
ode/src/capsule.cpp
ode/src/collision_cylinder_sphere.cpp
ode/src/collision_sapspace.cpp
ode/src/lcp.cpp
ode/src/collision_kernel.cpp
ode/src/ode.cpp
ode/src/odeou.cpp
ode/src/collision_trimesh_trimesh_new.cpp
ode/src/obstack.cpp
ode/src/collision_trimesh_distance.cpp
ode/src/collision_util.cpp
ode/src/odeinit.cpp
ode/src/odetls.cpp
ode/src/testing.cpp
ode/src/matrix.cpp
ode/src/mass.cpp
ode/src/collision_trimesh_ray.cpp
ode/src/timer.cpp
ode/src/misc.cpp
ode/src/quickstep.cpp
ode/src/collision_trimesh_plane.cpp
ode/src/array.cpp
ode/src/error.cpp
ode/src/sphere.cpp
ode/src/collision_trimesh_trimesh.cpp
ode/src/memory.cpp
ode/src/joints/pu.cpp
ode/src/joints/joint.cpp
ode/src/joints/hinge2.cpp
ode/src/joints/lmotor.cpp
ode/src/joints/null.cpp
ode/src/joints/piston.cpp
ode/src/joints/fixed.cpp
ode/src/joints/hinge.cpp
ode/src/joints/universal.cpp
ode/src/joints/amotor.cpp
ode/src/joints/ball.cpp
ode/src/joints/pr.cpp
ode/src/joints/contact.cpp
ode/src/joints/plane2d.cpp
ode/src/joints/slider.cpp
ode/src/collision_cylinder_trimesh.cpp
ode/src/rotation.cpp
ode/src/mat.cpp
ode/src/collision_trimesh_box.cpp
ode/src/collision_trimesh_sphere.cpp
ode/src/ray.cpp
ode/src/collision_trimesh_ccylinder.cpp
ode/src/plane.cpp
ode/src/stepfast.cpp
ode/src/box.cpp
ode/src/collision_trimesh_gimpact.cpp
ode/src/util.cpp
ode/src/collision_space.cpp
ode/src/collision_transform.cpp
ode/src/convex.cpp
ode/src/collision_quadtreespace.cpp
ode/src/odemath.cpp
ode/src/collision_cylinder_box.cpp
ode/src/collision_trimesh_disabled.cpp
ode/src/heightfield.cpp
ode/src/collision_cylinder_plane.cpp
ode/src/collision_trimesh_opcode.cpp
ode/src/export-dif.cpp
ode/src/cylinder.cpp
ode/src/step.cpp
ode/src/fastlsolve.c
ode/src/fastldlt.c
ode/src/fastdot.c
ode/src/fastltsolve.c
ode/demo/demo_chain1.c)
include_directories (.)
add_library (ode STATIC ${SRC})
add_executable (demo ode/demo/demo_boxstack.cpp)
target_link_libraries(demo GL GLU ode pthread)


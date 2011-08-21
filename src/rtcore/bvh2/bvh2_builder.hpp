// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#ifndef __PF_BVH2_BUILDER_H__
#define __PF_BVH2_BUILDER_H__

#include "bvh2.hpp"
#include "../bvh4/triangle4.hpp"
#include "../common/builder.hpp"
#include "../common/object_binning.hpp"
#include "../common/object_binning_parallel.hpp"

namespace pf
{
  /* BVH2 builder. The builder is multi-threaded and implements 3
   * different build strategies. 1) Small tasks are finished in a
   * single thread (BuildTask) 2) Medium sized tasks are split into
   * two tasks using a single thread (SplitTask) and 3) Large tasks
   * are split into two tasks in a parallel fashion
   * (ParallelSplitTask). */

  class BVH2Builder : private Builder
  {
  public:

    /*! API entry function for the builder */
    static Ref<BVH2<Triangle4> > build(const BuildTriangle* triangles, size_t numTriangles);

  public:

    /*! Constructs the builder. */
    BVH2Builder(const BuildTriangle* triangles, size_t numTriangles, Ref<BVH2<Triangle4> > bvh);

    /*! Selects between full build, single-threaded split, and multi-threaded split strategy. */
    void recurse(int& nodeID, size_t depth, const BuildRange& job);

    /*! Selects between full build and single-threaded split strategy. */
    void recurse(int& nodeID, size_t depth, const ObjectBinning<2>& job);

    /*! Computes the number of blocks of a number of triangles. */
    static INLINE size_t blocks(size_t x) { return (x+3)/4; }

    /*! Single-threaded task that builds a complete BVH. */
    class BuildTask {
      ALIGNED_CLASS
    public:

      /*! Default task construction. */
      BuildTask(BVH2Builder* parent, int& nodeID, size_t depth, const ObjectBinning<2>& job);

      /*! Task entry function. */
      static void run(size_t tid, BuildTask* This, size_t elts);

      /*! Recursively finishes the BVH construction. */
      int recurse(size_t depth, ObjectBinning<2>& job);

    private:
      BVH2Builder* parent;   //!< Pointer to parent task.
      size_t       tid;      //!< Task ID for fast thread local storage.
      int&         nodeID;   //!< Reference to output the node ID.
      size_t       depth;    //!< Recursion depth of the root of this subtree.
      ObjectBinning<2> job;  //!< Binner for performing splits.
    };

    /*! Single-threaded task that builds a single node and creates subtasks for the children. */
    class SplitTask {
      ALIGNED_CLASS
    public:

      /*! Default task construction. */
      SplitTask(BVH2Builder* parent, int& nodeID, size_t depth, const ObjectBinning<2>& job);

      /*! Task entry function. */
      void split(); static void _split(size_t tid, SplitTask* This, size_t elts) { This->split(); }

    private:
      BVH2Builder* parent;   //!< Pointer to parent task.
      int&         nodeID;   //!< Reference to output the node ID.
      size_t       depth;    //!< Recursion depth of this node.
      ObjectBinning<2> job;  //!< Binner for performing splits.
    };

    /*! Multi-threaded task that builds a single node and creates
     *  subtasks for the children. The box array has to have twice the
     *  size of the triangle array, as the parallel binner does not
     *  split in-place. It has to copy the boxes from the left half to
     *  the right half and vice versa. */

    class ParallelSplitTask {
      ALIGNED_CLASS
    public:

      /*! Default task construction. */
      ParallelSplitTask(BVH2Builder* parent, int& nodeID, size_t depth, const BuildRange& job);

      /*! Compute target location for parallel binning job. It the
       *  boxes are in the left half of the box array we copy into the
       *  right half and vice versa. */
      INLINE size_t target(const BuildRange& r) {
        return r.start() < parent->numTriangles ? r.start()+parent->numTriangles : r.start()-parent->numTriangles;
      }

      /*! Called after the parallel binning to creates the node. */
      void createNode(size_t tid); static void _createNode(size_t tid, ParallelSplitTask* This) { This->createNode(tid); }

    private:
      BVH2Builder* parent;             //!< Pointer to parent task.
      int&         nodeID;             //!< Reference to output the node ID.
      size_t       depth;              //!< Recursion depth of this node.
      ObjectBinningParallel<2> binner; //!< Parallel Binner
    };

  public:
    const BuildTriangle* triangles;     //!< Source triangle array
    size_t numTriangles;                //!< Number of triangles
    Box* prims;                         //!< Working array. Build tasks operate on ranges in this array. */
    Ref<BVH2<Triangle4> > bvh;          //!< BVH to overwrite
  };
}

#endif

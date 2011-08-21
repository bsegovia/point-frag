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

#ifndef __PF_BVH2_BUILDER_SPATIAL_H__
#define __PF_BVH2_BUILDER_SPATIAL_H__

#include "bvh2.hpp"
#include "../bvh4/triangle4.hpp"
#include "../common/builder.hpp"
#include "../common/spatial_binning.hpp"

namespace pf
{
  /* BVH2 spatial split builder. The builder uses a combined object
   * split and spatial splitting strategy. Object splits are evaluated
   * through binning like in the BVH2Builder, however, a central
   * spatial split in each dimension is considered too. The best of
   * all these splitting possibilities is choosen. Spatial splits
   * cause the number of triangle references to increase through split
   * triangles. The builder bounds this tiangle duplication through
   * the duplicationFactor constant. Through breadth-first
   * construction, the builder is able to distribute the spatial
   * splits evenly over the scene until the maximal triangle
   * duplication is reached. Passes of this breadth-first construction
   * sort primitives from left-to-right and right-to-left in the
   * primitive array. This makes memory management trivial and allows
   * to compactly fill the primitive array. The builder is
   * multi-threaded by first performing a number of single threaded
   * build iterations for the high nodes of the tree (BuildTaskHigh)
   * and then distributing work to multiple threads to build the lower
   * nodes of the tree (BuildTaskLow). */

  class BVH2BuilderSpatial : private Builder
  {
  public:

    /*! Maximal duplication of triangles through spatial splits. */
    static const float duplicationFactor;

    /*! API entry function for the builder */
    static Ref<BVH2<Triangle4> > build(const BuildTriangle* triangles, size_t numTriangles);

  public:

    /*! Constructs the builder. */
    BVH2BuilderSpatial(const BuildTriangle* triangles, size_t numTriangles, Ref<BVH2<Triangle4> > bvh);

    /*! Computes the number of blocks of a number of triangles. */
    static INLINE size_t blocks(size_t x) { return (x+3)/4; }

    /*! Single-threaded breadth-first build task. */
    class BuildTaskLow {
      ALIGNED_CLASS
    public:

      /*! Default task construction. */
      BuildTaskLow(BVH2BuilderSpatial* parent, size_t primBegin, size_t primEnd, size_t jobBegin, size_t jobEnd, size_t numJobs);

      /*! Task entry function. */
      static void run(size_t tid, BuildTaskLow* This, size_t elts);

      /*! Recursively finishes the BVH construction. */
      void build();

    private:
      BVH2BuilderSpatial* parent;   //!< Pointer to parent task.
      size_t       tid;             //!< Task ID for fast thread local storage.
      size_t       primBegin;       //!< Beginning of assigned range in primitive array.
      size_t       primEnd;         //!< End of assigned range in primitive array.
      size_t       jobBegin;        //!< Beginning of assigned range in job array.
      size_t       jobEnd;          //!< End of assigned range in job array.
      size_t       numJobs;         //!< Number of jobs assigned to this task.
      int*         roots[128];      //!< Root nodes of assigned jobs (for later tree rotations)
    };

    /*! Single-threaded breadth-first build task. */
    class BuildTaskHigh {
      ALIGNED_CLASS
    public:

      /*! Default task construction. */
      BuildTaskHigh(BVH2BuilderSpatial* parent, size_t primBegin, size_t primEnd, size_t jobBegin, size_t jobEnd, size_t numJobs);

      /*! Task entry function. */
      static void run(size_t tid, BuildTaskHigh* This, size_t elts);

      /*! Recursively finishes the BVH construction. */
      void build();

    private:
      BVH2BuilderSpatial* parent;       //!< Pointer to parent task.
      size_t       primBegin;           //!< Beginning of assigned range in primitive array.
      size_t       primEnd;             //!< End of assigned range in primitive array.
      size_t       jobBegin;            //!< Beginning of assigned range in job array.
      size_t       jobEnd;              //!< End of assigned range in job array.
      size_t       numJobs;             //!< Number of jobs assigned to this task.
    };

  private:
    size_t numThreads;                  //!< Number of threads used by the builder.
    size_t numTriangles;                //!< Number of triangles
    const BuildTriangle* triangles;     //!< Source triangle array
    Box* prims;                         //!< Working array referenced by ranges of primitives. */
    SpatialBinning<2>* jobs;            //!< Working array referenced by ranges of build jobs. */
    Ref<BVH2<Triangle4> > bvh;          //!< output BVH
  };
}

#endif

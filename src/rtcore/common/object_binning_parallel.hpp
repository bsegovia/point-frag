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

#ifndef __PF_OBJECT_BINNING_PARALLEL_HPP__
#define __PF_OBJECT_BINNING_PARALLEL_HPP__

#include "build_range.hpp"

namespace pf
{
  /* Multi threaded object binner. Finds the split with the best SAH
   * heuristic by testing for each dimension multiple partitionings
   * for regular spaced partition locations. A partitioning for a
   * partition location is computed, by putting primitives whose
   * centroid is on the left and right of the split location to
   * different sets. The SAH is evaluated by computing the number of
   * blocks occupied by the primitives in the partitions. */

  template<int logBlockSize>
    class ObjectBinningParallel : public BuildRange
  {
    /*! Maximal number of bins. */
    enum { maxBins = 32 };

  public:

    /*! Default constructor. */
    INLINE ObjectBinningParallel() {}

    /*! Construct from build range. */
    ObjectBinningParallel(const BuildRange& range, size_t target, Box* prims);

    /*! Start parallel binning. */
    void go(Task::completeFunction continuation, void* data);

  private:

    /*! Multi-threaded binning stage. Maps geometry into bins. Each bin stores number of primitives and bounds. */
    void stage0(size_t elt); static void _stage0(size_t tid, ObjectBinningParallel* This, size_t elt) { This->stage0(elt); }

    /*! Finds the best splitting dimension and location. */
    void stage1(          ); static void _stage1(size_t tid, ObjectBinningParallel* This            ) { This->stage1(   ); }

    /*! Multi-threaded split stage. Performs the best split, by copying the elements to a new location. */
    void stage2(size_t elt); static void _stage2(size_t tid, ObjectBinningParallel* This, size_t elt) { This->stage2(elt); }

    /*! Merges the bounds of the geometry in the left and right partition. */
    void stage3(size_t tid); static void _stage3(size_t tid, ObjectBinningParallel* This            ) { This->stage3(tid); }

  private:

    /*! compute the bin during binning */
    INLINE ssei getBin(const Box& box) const { return clamp(ssei((center2(box) - centBounds.lower)*scale-0.5f),ssei(0),ssei((int)numBins-1)); }
    INLINE ssei getBin(const ssef& c ) const { return ssei((c-centBounds.lower)*scale - 0.5f); }

     /*! compute the number of SIMD blocks of a number */
    INLINE ssei blocks(const ssei& a) { return (a+ssei((1 << logBlockSize)-1)) >> logBlockSize; }
    INLINE int blocks(size_t a) { return int((a+((1LL << logBlockSize)-1)) >> logBlockSize); }

  private:
    Box* prims_i;                         //!< Source array.
    Box* prims_o;                         //!< Target array.
    size_t target;                        //!< Offset of target location.
    size_t numBins;                       //!< Actual number of bins to use.
    ssef scale;                           //!< Scaling factor to compute bin.

    /* output of parallel binning stage (stage0) */
    struct Thread {
      ssei binLeftCount[maxBins];         //!< Number of primitives on the left of split.
      ssei binRightCount[maxBins];        //!< Number of primitives on the right of split.
      Box binBounds[maxBins][4];          //!< Bounds for every bin in every dimension.
      Box lgeomBounds;                    //!< Geometry bounds of geometry left of split.
      Box lcentBounds;                    //!< Centroid bounds of geometry left of split.
      Box rgeomBounds;                    //!< Geometry bounds of geometry right of split.
      Box rcentBounds;                    //!< Centroid bounds of geometry right of split.
    } thread[8];

    /* output of find best splti stage (stage1) */
    int bestDim;                          //!< Best splitting dimension
    int bestSplit;                        //!< Best splitting location
    size_t numLeft;                       //!< Number of primitives on the left of best split
    size_t numRight;                      //!< Number of primitives on the right of the best split
    size_t targetLeft[8];                 //!< Target offset for the n-th thread to put left primitives.
    size_t targetRight[8];                //!< Target offset for the n-th thread to put right primitives.

  public:
    float splitSAH;                        //!< SAH cost for the best split.
    float leafSAH;                         //!< SAH cost for creating a leaf.
    BuildRange left;                       //!< Build job to continue with left geometry.
    BuildRange right;                      //!< Build job to continue with right geometry.

  private:
    Task::completeFunction continuation;   //!< Continuation function
    void* data;                            //!< Argument to call continuation with
  };
}

#endif


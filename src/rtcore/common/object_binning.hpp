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

#ifndef __PF_OBJECT_BINNING_H__
#define __PF_OBJECT_BINNING_H__

#include "build_range.hpp"

namespace pf
{
  /* Single threaded object binner. Finds the split with the best SAH
   * heuristic by testing for each dimension multiple partitionings
   * for regular spaced partition locations. A partitioning for a
   * partition location is computed, by putting primitives whose
   * centroid is on the left and right of the split location to
   * different sets. The SAH is evaluated by computing the number of
   * blocks occupied by the primitives in the partitions. */

  template<int logBlockSize>
    class ObjectBinning : public BuildRange
  {
    /*! Maximal number of bins. */
    enum { maxBins = 32 };

  public:

    /*! Default constructor. */
    INLINE ObjectBinning() {}

    /*! Construct from build range. The constructor will directly find the best split. */
    ObjectBinning(const BuildRange& job, Box* prims);

    /*! Execute the best found split. */
    void split(Box* prims, ObjectBinning& left_o, ObjectBinning& right_o) const;

  private:

    /*! Computes the bin numbers for each dimension for a box. */
    INLINE ssei getBin(const Box& box) const { return clamp(ssei((center2(box) - centBounds.lower)*scale-0.5f),ssei(0),ssei((int)numBins-1)); }

    /*! Computes the bin numbers for each dimension for a point. */
    INLINE ssei getBin(const ssef& c ) const { return ssei((c-centBounds.lower)*scale - 0.5f); }

    /*! Compute the number of blocks occupied for each dimension. */
    INLINE ssei blocks(const ssei& a) const { return (a+ssei((1 << logBlockSize)-1)) >> logBlockSize; }

    /*! Compute the number of blocks occupied in one dimension. */
    INLINE int  blocks(size_t a) const { return (int)((a+((1LL << logBlockSize)-1)) >> logBlockSize); }

  private:
    size_t numBins;   //!< Actual number of bins to use.
    ssef   scale;     //!< Scaling factor to compute bin.
  public:
    float splitSAH;   //!< SAH cost of the best split.
  private:
    int dim;          //!< Best split dimension.
    int pos;          //!< Best split position.
  public:
    float leafSAH;    //!< SAH cost of creating a leaf.
  };
}

#endif


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

#ifndef __PF_SPATIAL_BINNING_H__
#define __PF_SPATIAL_BINNING_H__

#include "rtcore/rtcore.hpp"
#include "build_range.hpp"

namespace pf
{
  /* Single threaded combined object and spatial binner. Performs the
   * same object binning procedure as the ObjectBinner class. In
   * addition tries spatial splits, by splitting each dimension
   * spatially in the center. In contrast to object binning, spatial
   * binning potentially cuts triangles along the splitting plane and
   * sortes the corresponding parts into the left and right set. The
   * splitting functions allow primitives to be moved from the left of
   * the primitive array to the right and vice versa. When moving from
   * left-to-right the source is shrinked from the right and the
   * destination expanded towards the left (analogous for
   * right-to-left splits). This allows splits to be performed even
   * when only the number of duplicates of additional space is
   * available in the primitive array. */

  template<int logBlockSize>
  class SpatialBinning : public BuildRange
  {
    /*! Maximal number of bins for object binning. Spatial binning
     *  only considers center. */
    enum { maxBins = 32 };

  public:

    /*! Default constructor. */
    INLINE SpatialBinning() {}

    /*! Construct from build range. The constructor will directly find the best split. */
    SpatialBinning(const BuildRange& job, Box* prims, const BuildTriangle* triangles, size_t depth);

    /*! Split the list and move primitives from the left side to the right side of the primitive array. */
    void split_l2r(bool spatial, Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& rprim) const;

    /*! Split the list and move primitives from the right side to the left side of the primitive array. */
    void split_r2l(bool spatial, Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& lprim) const;

  private:

    /*! Perform object split and move primitives from the left side to the right side of the primitive array. */
    void object_split_l2r(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& rprim) const;

    /*! Perform object split and move primitives from the right side to the left side of the primitive array. */
    void object_split_r2l(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& lprim) const;

    /*! Perform spatial split and move primitives from the left side to the right side of the primitive array. */
    void spatial_split_l2r(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& rprim) const;

    /*! Perform spatial split and move primitives from the right side to the left side of the primitive array. */
    void spatial_split_r2l(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& lprim) const;

    /*! Computes number of bins to use. */
    INLINE size_t computeNumBins() const { return min(size_t(maxBins),size_t(4.0f + 0.05f*size())); }

    /*! Computes the bin numbers for each dimension for a box. */
    INLINE ssei getBin(const Box& box, const ssef& scale, size_t numBins) const {
      return clamp(ssei((center2(box) - centBounds.lower)*scale-0.5f),ssei(0),ssei((int)numBins-1));
    }

    /*! Computes the bin numbers for each dimension for a point. */
    INLINE ssei getBin(const ssef& c, const ssef& scale, size_t numBins) const {
      return ssei((c-centBounds.lower)*scale - 0.5f);
    }

    /*! Compute the number of blocks occupied for each dimension. */
    INLINE ssei blocks(const ssei& a) const { return (a+ssei((1 << logBlockSize)-1)) >> logBlockSize; }

    /*! Compute the number of blocks occupied in one dimension. */
    INLINE int  blocks(size_t a) const { return (int)((a+((1LL << logBlockSize)-1)) >> logBlockSize); }

    /*! Clipping code. Splits a triangle with a clipping plane into two halves. */
    std::pair<Box,Box> splitBox(const BuildTriangle* triangles, const Box& ref, int dim, float pos) const;

  public:
    char depth;           //!< Tree depth of this job was generated at.
    char objectDim;       //!< Best object split dimension
    char spatialDim;      //!< Best spatial split dimension
    char objectPos;       //!< Best object split position
    float leafSAH;        //!< SAH cost of creating a leaf
    float objectSAH;      //!< SAH cost of performing best object split
    float spatialSAH;     //!< SAH cost of performing best spatial split
    int spatialSize;      //!< Maximal number of primitive space required for performing the split.
    int* node;            //!< Target node.
  };
}

#endif

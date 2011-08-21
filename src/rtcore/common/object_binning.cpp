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

#include "object_binning.hpp"

namespace pf
{
  template<int logBlockSize>
  ObjectBinning<logBlockSize>::ObjectBinning(const BuildRange& job, Box* prims)
    : BuildRange(job), splitSAH(inf), dim(0), pos(0)
  {
    /*! compute number of bins to use and precompute scaling factor for binning */
    numBins = min(size_t(maxBins),size_t(4.0f + 0.05f*size()));
    scale = rcp(pf::size(centBounds)) * ssef((float)numBins);

    /*! initialize binning counter and bounds */
    Box binBounds[maxBins][4]; //!< bounds for every bin in every dimension
    ssei binCount[maxBins];    //!< number of primitives mapped to bin
    for (size_t i=0; i<numBins; i++) {
      binCount[i] = 0;
      binBounds[i][0] = binBounds[i][1] = binBounds[i][2] = empty;
    }

    /*! map geometry to bins, unrolled once */
    {
      index_t i;
      for (i=0; i<index_t(size())-1; i+=2)
      {
        prefetchL2(&prims[start()+i+8]);

        /*! map even and odd primitive to bin */
        Box prim0 = prims[start()+i+0]; ssei bin0 = getBin(prim0);
        Box prim1 = prims[start()+i+1]; ssei bin1 = getBin(prim1);

        /*! increase bounds for bins for even primitive */
        int b00 = extract<0>(bin0); binCount[b00][0]++; binBounds[b00][0].grow(prim0);
        int b01 = extract<1>(bin0); binCount[b01][1]++; binBounds[b01][1].grow(prim0);
        int b02 = extract<2>(bin0); binCount[b02][2]++; binBounds[b02][2].grow(prim0);

        /*! increase bounds of bins for odd primitive */
        int b10 = extract<0>(bin1); binCount[b10][0]++; binBounds[b10][0].grow(prim1);
        int b11 = extract<1>(bin1); binCount[b11][1]++; binBounds[b11][1].grow(prim1);
        int b12 = extract<2>(bin1); binCount[b12][2]++; binBounds[b12][2].grow(prim1);
      }

      /*! for uneven number of primitives */
      if (i < index_t(size()))
      {
        /*! map primitive to bin */
        Box prim0 = prims[start()+i]; ssei bin0 = getBin(prim0);

        /*! increase bounds of bins */
        int b00 = extract<0>(bin0); binCount[b00][0]++; binBounds[b00][0].grow(prim0);
        int b01 = extract<1>(bin0); binCount[b01][1]++; binBounds[b01][1].grow(prim0);
        int b02 = extract<2>(bin0); binCount[b02][2]++; binBounds[b02][2].grow(prim0);
      }
    }

    /*! sweep from right to left and compute parallel prefix of merged bounds */
    ssef rArea[maxBins];       //!< area of bounds of primitives on the right
    ssef rCount[maxBins];      //!< number of primitives on the right
    ssei count = 0; Box bx = empty, by = empty, bz = empty;
    for (size_t i=numBins-1; i>0; i--)
    {
      count = count + binCount[i];
      rCount[i] = ssef(blocks(count));
      bx = merge(bx,binBounds[i][0]); rArea[i][0] = halfArea(bx);
      by = merge(by,binBounds[i][1]); rArea[i][1] = halfArea(by);
      bz = merge(bz,binBounds[i][2]); rArea[i][2] = halfArea(bz);
    }

    /*! sweep from left to right and compute SAH */
    ssei ii = 1;
    ssef bestSAH = pos_inf;
    ssei bestSplit = -1;
    count = 0; bx = empty; by = empty; bz = empty;
    for (size_t i=1; i<numBins; i++, ii+=1)
    {
      count = count + binCount[i-1];
      bx = merge(bx,binBounds[i-1][0]); float Ax = halfArea(bx);
      by = merge(by,binBounds[i-1][1]); float Ay = halfArea(by);
      bz = merge(bz,binBounds[i-1][2]); float Az = halfArea(bz);
      ssef lCount = ssef(blocks(count));
      ssef lArea = ssef(Ax,Ay,Az,Az);
      ssef sah = lArea*lCount + rArea[i]*rCount[i];
      bestSplit = select(sah < bestSAH,ii,bestSplit);
      bestSAH = min(sah,bestSAH);
    }
    bestSAH = insert<3>(select(pf::size(centBounds) <= ssef(zero),ssef(inf),bestSAH), inf);

    /*! find best dimension */
    dim = (int)__bsf(movemask(reduce_min(bestSAH) == bestSAH));
    splitSAH = bestSAH[dim];
    pos = bestSplit[dim];
    leafSAH  = halfArea(geomBounds)*blocks(size());
  }

  template<int logBlockSize>
  void ObjectBinning<logBlockSize>::split(Box* prims, ObjectBinning& left_o, ObjectBinning& right_o) const
  {
    size_t N = size();
    Box lgeomBounds = empty; Box lcentBounds = empty;
    Box rgeomBounds = empty; Box rcentBounds = empty;

    index_t l = 0, r = N-1;
    while (l <= r) {
      prefetchL2(&prims[start()+l+8]);
      prefetchL2(&prims[start()+r-8]);
      Box prim = prims[start()+l];
      ssef center = center2(prim);
      if (getBin(center)[dim] < pos) {
        lgeomBounds.grow(prim);
        lcentBounds.grow(center);
        l++;
      }
      else {
        rgeomBounds.grow(prim);
        rcentBounds.grow(center);
        std::swap(prims[start()+l],prims[start()+r]);
        r--;
      }
    }

    /*! finish */
    if (l != 0 && N-1-r != 0)
    {
      new (&right_o) ObjectBinning (BuildRange(start()+l,N-1-r,rgeomBounds,rcentBounds),prims);
      new (&left_o ) ObjectBinning (BuildRange(start()  ,l    ,lgeomBounds,lcentBounds),prims);
      return;
    }

    /*! object medium split if we did not make progress, can happen when all primitives have same centroid */
    lgeomBounds = empty; lcentBounds = empty;
    rgeomBounds = empty; rcentBounds = empty;

    for (size_t i=0; i<N/2; i++) {
      lgeomBounds.grow(prims[start()+i]);
      lcentBounds.grow(center2(prims[start()+i]));
    }
    for (size_t i=N/2; i<N; i++) {
      rgeomBounds.grow(prims[start()+i]);
      rcentBounds.grow(center2(prims[start()+i]));
    }

    new (&right_o) ObjectBinning (BuildRange(start()+N/2,N/2+N%2,rgeomBounds,rcentBounds),prims);
    new (&left_o ) ObjectBinning (BuildRange(start()    ,N/2    ,lgeomBounds,lcentBounds),prims);
  }

  /*! explicit template instantiations */
  template class ObjectBinning<2>;
}


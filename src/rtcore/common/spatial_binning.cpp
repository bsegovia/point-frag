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

#include "spatial_binning.hpp"
#include <algorithm>

namespace pf
{
  /*! Primitives whose right bound is smaller go to the left size. */
  const float leftBound = 0.51f;

  /*! Primitives whose left bound is larger go to the right size. */
  const float rightBound = 0.49f;

  template<int logBlockSize>
  INLINE std::pair<Box,Box> SpatialBinning<logBlockSize>::splitBox(const BuildTriangle* triangles, const Box& box, int dim, float pos) const
  {
    std::pair<Box,Box> pair(empty,empty);
    const BuildTriangle& tri = triangles[box.lower.i[3]];

    /* clip triangle to left and right box by processing all edges */
    ssef v1 = tri[2];
    for (index_t i=0; i<3; i++)
    {
      ssef v0 = v1; v1 = tri[i];
      float v0d = v0[dim], v1d = v1[dim];

      if (v0d <= pos) pair.first .grow(v0); // this point is on left side
      if (v0d >= pos) pair.second.grow(v0); // this point is on right side

      if ((v0d < pos && v1d > pos) || (v0d > pos && v1d < pos)) // the edge crosses the splitting location
      {
        float t = clamp((pos-v0d)*rcp(v1d-v0d),0.0f,1.0f);
        ssef c = v0*(1.0f-t) + v1*t;
        pair.first.grow(c);
        pair.second.grow(c);
      }
    }

    pair.first  = intersect(pair.first ,box);  pair.first .lower.i[3] = box.lower.i[3];
    pair.second = intersect(pair.second,box);  pair.second.lower.i[3] = box.lower.i[3];
    return pair;
  }

  template<int logBlockSize>
  SpatialBinning<logBlockSize>::SpatialBinning(const BuildRange& job, Box* prims, const BuildTriangle* triangles, size_t depth)
    : BuildRange(job), depth((char)depth), objectDim(0), spatialDim(0), objectPos(0), leafSAH(inf), objectSAH(inf), spatialSAH(inf), spatialSize(0), node(NULL)
  {
    /*! compute number of bins to use and precompute scaling factor for binning */
    size_t numBins = computeNumBins();
    ssef scale = rcp(pf::size(centBounds)) * ssef((float)numBins);

    /* initialize binning counter and bounds */
    Box binBounds[maxBins][4]; //< bounds for every bin in every dimension
    ssei binCount[maxBins];    //< number of primitives mapped to bin

    for (size_t i=0; i<numBins; i++) {
      binCount[i] = 0;
      binBounds[i][0] = binBounds[i][1] = binBounds[i][2] = empty;
    }

    /*! initialize counters for spatial split */
    Box lspatialBounds[4], rspatialBounds[4];  //< left and right bounds for spatial split
    ssei lspatialCount   , rspatialCount;      //< left and right count for spatial split
    lspatialBounds[0] = lspatialBounds[1] = lspatialBounds[2] = empty;
    rspatialBounds[0] = rspatialBounds[1] = rspatialBounds[2] = empty;
    lspatialCount = rspatialCount = 0;

    /* map geometry to bins */
    for (size_t i=start(); i<end(); i++)
    {
      /*! map primitive to bin */
      Box prim = prims[i];
      ssei bin = getBin(prim,scale,numBins);

      /*! increase bounds of bins */
      uint32 b00 = extract<0>(bin); binCount[b00][0]++; binBounds[b00][0].grow(prim);
      uint32 b01 = extract<1>(bin); binCount[b01][1]++; binBounds[b01][1].grow(prim);
      uint32 b02 = extract<2>(bin); binCount[b02][2]++; binBounds[b02][2].grow(prim);

      /*! Test spatial split in center of each dimension. */
      ssef center = 0.5f*center2(geomBounds);
      ssef bleft  = geomBounds.lower + leftBound *pf::size(geomBounds);
      ssef bright = geomBounds.lower + rightBound*pf::size(geomBounds);
      sseb left  = prim.upper <= bleft;
      sseb right = prim.lower >= bright;

      for (int maxDim=0; maxDim<3; maxDim++)
      {
        /*! Choose better side for primitives between rightBound and leftBound. */
        if (left[maxDim] && right[maxDim]) {
          if (prim.upper[maxDim]-bright[maxDim] > bleft[maxDim]-prim.lower[maxDim]) {
            rspatialBounds[maxDim].grow(prim); rspatialCount[maxDim]++;
          } else {
            lspatialBounds[maxDim].grow(prim); lspatialCount[maxDim]++;
          }
        }
        /*! These definitely go to the left. */
        if (left[maxDim]) {
          lspatialBounds[maxDim].grow(prim); lspatialCount[maxDim]++;
        }
        /*! These definitely go to the right. */
        else if (right[maxDim]) {
          rspatialBounds[maxDim].grow(prim); rspatialCount[maxDim]++;
        }
        /*! These have to get split and put to left and right. */
        else {
          std::pair<Box,Box> pair = splitBox(triangles,prim,maxDim,center[maxDim]);
          lspatialBounds[maxDim].grow(pair.first ); lspatialCount[maxDim]++;
          rspatialBounds[maxDim].grow(pair.second); rspatialCount[maxDim]++;
        }
      }
    }

    /* Compute SAH for best spatial split. */
    for (int i=0; i<3; i++) {
      float sah = halfArea(lspatialBounds[i])*float(blocks(lspatialCount[i]))+halfArea(rspatialBounds[i])*float(blocks(rspatialCount[i]));
      if (sah < spatialSAH) {
        spatialSAH = sah;
        spatialDim = (char)i;
        spatialSize = lspatialCount[i]+rspatialCount[i];
      }
    }

    /* sweep from right to left and compute parallel prefix of merged bounds */
    ssef rArea[maxBins];       //< area of bounds of primitives on the right
    ssef rCount[maxBins];      //< number of primitives on the right
    ssei count = 0; Box bx = empty; Box by = empty; Box bz = empty;
    for (size_t i=numBins-1; i>0; i--)
    {
      count = count + binCount[i];
      rCount[i] = ssef(blocks(count));
      bx = merge(bx,binBounds[i][0]); rArea[i][0] = halfArea(bx);
      by = merge(by,binBounds[i][1]); rArea[i][1] = halfArea(by);
      bz = merge(bz,binBounds[i][2]); rArea[i][2] = halfArea(bz);
    }

    /* sweep from left to right and compute SAH */
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

    /*! set SAH of building a leaf node */
    leafSAH  = halfArea(geomBounds)*blocks(int(size()));

    /* set SAH for best object split */
    objectDim = (char)__bsf(movemask(reduce_min(bestSAH) == bestSAH));
    objectSAH = bestSAH[objectDim];
    objectPos = (char)bestSplit[objectDim];
  }

  template<int logBlockSize>
  void SpatialBinning<logBlockSize>::object_split_l2r(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& rprim) const
  {
    size_t numBins = computeNumBins();
    ssef scale = rcp(pf::size(centBounds)) * ssef((float)numBins);

    /*! Left primitives are in [lstart,rstart[, Right primitives are
     *  in [rstart,rend[. Grows to the left. */
    size_t lstart = rprim;
    size_t rstart = rprim;
    size_t rend   = rprim;

    Box lgeomBounds = empty; Box lcentBounds = empty;
    Box rgeomBounds = empty; Box rcentBounds = empty;

    for (index_t l=end()-1; l>=index_t(start()); l--)
    {
      Box prim = prims[l];
      ssef center = center2(prim);
      if (getBin(center,scale,numBins)[objectDim] < objectPos) {
        lgeomBounds.grow(prim);
        lcentBounds.grow(center);
        prims[--lstart] = prim;
      }
      else {
        rgeomBounds.grow(prim);
        rcentBounds.grow(center);
        prims[--lstart] = prims[rstart-1];
        prims[--rstart] = prim;
      }
    }

    new (&right_o) SpatialBinning (BuildRange(rstart,rend  -rstart,rgeomBounds,rcentBounds),prims,triangles,depth+1);
    new (&left_o ) SpatialBinning (BuildRange(lstart,rstart-lstart,lgeomBounds,lcentBounds),prims,triangles,depth+1);
    rprim = lstart;
  }

  template<int logBlockSize>
  void SpatialBinning<logBlockSize>::object_split_r2l(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& lprim) const
  {
    size_t numBins = computeNumBins();
    ssef scale = rcp(pf::size(centBounds)) * ssef((float)numBins);

    /*! Left primitives are in [lstart,lend[, Right primitives are
     *  in [lend,rend[. Grows to the right. */
    size_t lstart = lprim;
    size_t lend   = lprim;
    size_t rend   = lprim;

    Box lgeomBounds = empty; Box lcentBounds = empty;
    Box rgeomBounds = empty; Box rcentBounds = empty;

    for (size_t r=start(); r<end(); r++)
    {
      Box prim = prims[r];
      ssef center = center2(prim);
      if (getBin(center,scale,numBins)[objectDim] < objectPos) {
        lgeomBounds.grow(prim);
        lcentBounds.grow(center);
        prims[rend++] = prims[lend];
        prims[lend++] = prim;
      }
      else {
        rgeomBounds.grow(prim);
        rcentBounds.grow(center);
        prims[rend++] = prim;
      }
    }

    new (&right_o) SpatialBinning (BuildRange(lend  ,rend-lend  ,rgeomBounds,rcentBounds),prims,triangles,depth+1);
    new (&left_o ) SpatialBinning (BuildRange(lstart,lend-lstart,lgeomBounds,lcentBounds),prims,triangles,depth+1);
    lprim = rend;
  }

  template<int logBlockSize>
  void SpatialBinning<logBlockSize>::spatial_split_l2r(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& rprim) const
  {
    /*! Left primitives are in [lstart,rstart[, Right primitives are
     *  in [rstart,rend[. Grows to the left. */
    size_t lstart = rprim;
    size_t rstart = rprim;
    size_t rend   = rprim;

    /* current bounds of left and right child */
    Box leftBounds  = empty, leftCentBounds  = empty;
    Box rightBounds = empty, rightCentBounds = empty;

    /* split prims into, left, both, and right */
    float spatialPos = 0.5f*center2(geomBounds)[spatialDim];
    float bleft  = (geomBounds.lower + leftBound *pf::size(geomBounds))[spatialDim];
    float bright = (geomBounds.lower + rightBound*pf::size(geomBounds))[spatialDim];

    for (index_t l=end()-1; l>=index_t(start()); l--)
    {
      Box prim = prims[l];
      if (prim.upper[spatialDim] <= bleft) {
        leftBounds.grow(prim);
        leftCentBounds.grow(center2(prim));
        prims[--lstart] = prim;
      }
      else if (prim.lower[spatialDim] >= bright) {
        rightBounds.grow(prim);
        rightCentBounds.grow(center2(prim));
        prims[--lstart] = prims[rstart-1];
        prims[--rstart] = prim;
      } else {
        std::pair<Box,Box> pair = splitBox(triangles,prim,spatialDim,spatialPos);
        leftBounds = merge(leftBounds, pair.first);
        rightBounds = merge(rightBounds, pair.second);
        leftCentBounds .grow(center2(pair.first));
        rightCentBounds.grow(center2(pair.second));
        prims[--lstart] = prims[rstart-1];
        prims[--rstart] = pair.second;
        prims[--lstart] = pair.first;
      }
    }
    new (&right_o) SpatialBinning (BuildRange(rstart,rend  -rstart,rightBounds,rightCentBounds),prims,triangles,depth+1);
    new (&left_o ) SpatialBinning (BuildRange(lstart,rstart-lstart,leftBounds ,leftCentBounds ),prims,triangles,depth+1);
    rprim = lstart;
  }

  template<int logBlockSize>
  void SpatialBinning<logBlockSize>::spatial_split_r2l(Box* prims, const BuildTriangle* triangles, SpatialBinning& left_o, SpatialBinning& right_o, size_t& lprim) const
  {
    /*! Left primitives are in [lstart,lend[, Right primitives are
     *  in [lend,rend[. Grows to the right. */
    size_t lstart = lprim;
    size_t lend   = lprim;
    size_t rend   = lprim;

    /* current bounds of left and right child */
    Box leftBounds = empty,  leftCentBounds = empty;
    Box rightBounds = empty, rightCentBounds = empty;

    /* split prims into, left, both, and right */
    float spatialPos = 0.5f*center2(geomBounds)[spatialDim];
    float bleft  = (geomBounds.lower + leftBound *pf::size(geomBounds))[spatialDim];
    float bright = (geomBounds.lower + rightBound*pf::size(geomBounds))[spatialDim];

    for (size_t r=start(); r<end(); r++)
    {
      Box prim = prims[r];
      if (prim.upper[spatialDim] <= bleft) {
        leftBounds.grow(prim);
        leftCentBounds.grow(center2(prim));
        prims[rend++] = prims[lend];
        prims[lend++] = prim;
      }
      else if (prim.lower[spatialDim] >= bright) {
        rightBounds.grow(prim);
        rightCentBounds.grow(center2(prim));
        prims[rend++] = prim;
      } else {
        std::pair<Box,Box> pair = splitBox(triangles,prim,spatialDim,spatialPos);
        leftBounds = merge(leftBounds, pair.first);
        rightBounds = merge(rightBounds, pair.second);
        leftCentBounds .grow(center2(pair.first));
        rightCentBounds.grow(center2(pair.second));
        prims[rend++] = prims[lend];
        prims[lend++] = pair.first;
        prims[rend++] = pair.second;
      }
    }

    new (&right_o) SpatialBinning (BuildRange(lend  ,rend-lend  ,rightBounds,rightCentBounds),prims,triangles,depth+1);
    new (&left_o ) SpatialBinning (BuildRange(lstart,lend-lstart,leftBounds ,leftCentBounds ),prims,triangles,depth+1);
    lprim = rend;
  }

  template<int logBlockSize>
  void SpatialBinning<logBlockSize>::split_l2r(bool spatial, Box* prims, const BuildTriangle* triangles,
                                               SpatialBinning& left_o, SpatialBinning& right_o, size_t& rprim) const
  {
    if (spatial) spatial_split_l2r(prims,triangles,left_o,right_o,rprim);
    else         object_split_l2r (prims,triangles,left_o,right_o,rprim);
  }

  template<int logBlockSize>
  void SpatialBinning<logBlockSize>::split_r2l(bool spatial, Box* prims, const BuildTriangle* triangles,
                                               SpatialBinning& left_o, SpatialBinning& right_o, size_t& lprim) const
  {
    if (spatial) spatial_split_r2l(prims,triangles,left_o,right_o,lprim);
    else         object_split_r2l (prims,triangles,left_o,right_o,lprim);
  }

  /*! explicit template instantiations */
  template class SpatialBinning<2>;
}



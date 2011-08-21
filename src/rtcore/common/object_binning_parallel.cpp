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

#include "object_binning_parallel.hpp"

namespace pf
{
  template<int logBlockSize>
  INLINE ObjectBinningParallel<logBlockSize>::ObjectBinningParallel(const BuildRange& range, size_t target, Box* prims)
    : BuildRange(range), target(target), bestDim(0), bestSplit(0)
  {
    /*! compute number of bins to use and precompute scaling factor for binning */
    numBins = min(size_t(maxBins),size_t(4.0f + 0.05f*size()));
    scale = rcp(pf::size(centBounds)) * ssef((float)numBins);

    /*! pointer to source and target locations */
    prims_i = &prims[start()];
    prims_o = &prims[target];
  }

  template<int logBlockSize>
  void ObjectBinningParallel<logBlockSize>::go(Task::completeFunction continuation, void* data)
  {
    this->continuation = continuation;
    this->data = data;
    scheduler->addTask((Task::runFunction)_stage0,this,8,(Task::completeFunction)&_stage1,this);
  }

  template<int logBlockSize>
  void ObjectBinningParallel<logBlockSize>::stage0(size_t elt)
  {
    /* compute subrange to bin */
    index_t start = elt*size()/8, end = (elt+1)*size()/8;
    Thread& t = thread[elt];
    ssei binCount[maxBins];

    /* initialize binning counter and bounds */
    for (size_t i=0; i<numBins; i++) {
      binCount[i] = 0;
      t.binBounds[i][0] = t.binBounds[i][1] = t.binBounds[i][2] = empty;
    }

    /* map geometry to bins, unrolled once */
    {
      index_t i;
      for (i=start; i<end-1; i+=2)
      {
        prefetchL2(&prims_i[i+8]);

        /*! map even and odd primitive to bin */
        Box prim0 = prims_i[i];   ssei bin0 = getBin(prim0);
        Box prim1 = prims_i[i+1]; ssei bin1 = getBin(prim1);

        /*! increase bounds for bins for even primitive */
        uint32 b00 = extract<0>(bin0); binCount[b00][0]++; t.binBounds[b00][0].grow(prim0);
        uint32 b01 = extract<1>(bin0); binCount[b01][1]++; t.binBounds[b01][1].grow(prim0);
        uint32 b02 = extract<2>(bin0); binCount[b02][2]++; t.binBounds[b02][2].grow(prim0);

        /*! increase bounds of bins for odd primitive */
        uint32 b10 = extract<0>(bin1); binCount[b10][0]++; t.binBounds[b10][0].grow(prim1);
        uint32 b11 = extract<1>(bin1); binCount[b11][1]++; t.binBounds[b11][1].grow(prim1);
        uint32 b12 = extract<2>(bin1); binCount[b12][2]++; t.binBounds[b12][2].grow(prim1);
      }

      /*! for uneven number of primitives */
      if (i < end)
      {
        /*! map primitive to bin */
        Box prim0 = prims_i[i]; ssei bin0 = getBin(prim0);

        /*! increase bounds of bins */
        uint32 b00 = extract<0>(bin0); binCount[b00][0]++; t.binBounds[b00][0].grow(prim0);
        uint32 b01 = extract<1>(bin0); binCount[b01][1]++; t.binBounds[b01][1].grow(prim0);
        uint32 b02 = extract<2>(bin0); binCount[b02][2]++; t.binBounds[b02][2].grow(prim0);
      }
    }

    /* computes number of primitives on the right and left of a split location */
    ssei lcount = 0, rcount = 0;
    for (size_t i=1, j=numBins-1; i<numBins; i++, j--) {
      t.binLeftCount [i] = lcount += binCount[i-1];
      t.binRightCount[j] = rcount += binCount[j];
    }
  }

  template<int logBlockSize>
  void ObjectBinningParallel<logBlockSize>::stage1()
  {
    /* sweep from left to right (and right to left) and compute parallel prefix of merged bounds */
    ssef lArea [maxBins], rArea [maxBins];
    ssef lCount[maxBins], rCount[maxBins];
    ssei rStart[maxBins];
    Box lbx = empty, lby = empty, lbz = empty;
    Box rbx = empty, rby = empty, rbz = empty;
    for (size_t i=1, j=numBins-1; i<numBins; i++, j--) {
      ssei lcount = 0, rcount = 0;
      for (size_t elt=0; elt<8; elt++) {
        lcount += thread[elt].binLeftCount[i];
        lbx.grow(thread[elt].binBounds[i-1][0]);
        lby.grow(thread[elt].binBounds[i-1][1]);
        lbz.grow(thread[elt].binBounds[i-1][2]);
        rcount += thread[elt].binRightCount[j];
        rbx.grow(thread[elt].binBounds[j][0]);
        rby.grow(thread[elt].binBounds[j][1]);
        rbz.grow(thread[elt].binBounds[j][2]);
      }
      lArea[i][0] = halfArea(lbx);
      lArea[i][1] = halfArea(lby);
      lArea[i][2] = halfArea(lbz);
      lCount[i]   = ssef(blocks(lcount));
      rStart[i]   = lcount;
      rArea[j][0] = halfArea(rbx);
      rArea[j][1] = halfArea(rby);
      rArea[j][2] = halfArea(rbz);
      rCount[j]   = ssef(blocks(rcount));
    }

    /* sweep from left to right and compute SAH */
    ssei ii = 1;
    ssef bestSAH = inf;
    ssei bestSplits = -1;
    for (size_t i=1; i<numBins; i++, ii+=1) {
      ssef sah = lArea[i]*lCount[i] + rArea[i]*rCount[i];
      bestSplits = select(sah < bestSAH,ii,bestSplits);
      bestSAH = min(sah,bestSAH);
    }
    bestSAH = insert<3>(select(pf::size(centBounds) <= ssef(zero),ssef(inf),bestSAH), inf);

    /* find best dimension */
    bestDim = (int)__bsf(movemask(reduce_min(bestSAH) == bestSAH));
    splitSAH = bestSAH[bestDim];
    bestSplit = bestSplits[bestDim];
    leafSAH  = halfArea(geomBounds)*blocks(size());

    /* allocate space for parallel split stage */
    size_t nleft = 0, nright = 0;
    for (size_t elt=0; elt<8; elt++) {
      targetLeft[elt] = nleft;
      targetRight[elt] = rStart[bestSplit][bestDim]+nright;
      nleft += thread[elt].binLeftCount[bestSplit][bestDim];
      nright += thread[elt].binRightCount[bestSplit][bestDim];
    }
    numLeft = nleft;
    numRight = nright;

    /*! continue with parallel split stage */
    scheduler->addTask((Task::runFunction)_stage2,this,8,(Task::completeFunction)&_stage3,this);
  }

  template<int logBlockSize>
  void ObjectBinningParallel<logBlockSize>::stage2(size_t elt)
  {
    /* static work allocation */
    size_t start = elt*size()/8, end = (elt+1)*size()/8;
    Thread& t = thread[elt];

    /* split list and compute left and right bounds */
    size_t l = targetLeft[elt], r = targetRight[elt];
    Box lgeomBounds = empty, lcentBounds = empty;
    Box rgeomBounds = empty, rcentBounds = empty;

    for (size_t i=start; i<end; i++)
    {
      prefetchL2(&prims_i[i+8]);
      Box prim = prims_i[i];
      ssef center = center2(prim);
      ssef binf = (center-centBounds.lower)*scale - 0.5f;
      int bin = ssei(binf)[bestDim];
      if (bin < bestSplit) {
        lgeomBounds.grow(prim);
        lcentBounds.grow(center);
        prims_o[l++] = prim;
        prefetchL2(&prims_o[l+8]);
      }
      else {
        rgeomBounds.grow(prim);
        rcentBounds.grow(center);
        prims_o[r++] = prim;
        prefetchL2(&prims_o[r+8]);
      }
    }

    /* update context bounds */
    t.lgeomBounds = lgeomBounds;
    t.lcentBounds = lcentBounds;
    t.rgeomBounds = rgeomBounds;
    t.rcentBounds = rcentBounds;
  }

  template<int logBlockSize>
  void ObjectBinningParallel<logBlockSize>::stage3(size_t tid)
  {
    /*! merge bounds computed by splitjobs */
    size_t N = size();
    Box lgeomBounds = empty, lcentBounds = empty;
    Box rgeomBounds = empty, rcentBounds = empty;

    for (int i=0; i<8; i++) {
      lgeomBounds.grow(thread[i].lgeomBounds);
      rgeomBounds.grow(thread[i].rgeomBounds);
      lcentBounds.grow(thread[i].lcentBounds);
      rcentBounds.grow(thread[i].rcentBounds);
    }

    /*! finish */
    if (numLeft != 0 && numRight != 0)
    {
      new (&left ) BuildRange (target        ,numLeft ,lgeomBounds,lcentBounds);
      new (&right) BuildRange (target+numLeft,numRight,rgeomBounds,rcentBounds);
      continuation(tid,data);
      return;
    }

    /*! object medium split if we did not make progress, can happen if all primitives have the same centroid */
    lgeomBounds = empty; lcentBounds = empty;
    rgeomBounds = empty; rcentBounds = empty;

    for (size_t i=0; i<N/2; i++) {
      lgeomBounds.grow(prims_o[start()+i]);
      lcentBounds.grow(center2(prims_o[start()+i]));
    }
    for (size_t i=N/2; i<N; i++) {
      rgeomBounds.grow(prims_o[start()+i]);
      rcentBounds.grow(center2(prims_o[start()+i]));
    }

    new (&left ) BuildRange (target    ,N/2    ,lgeomBounds,lcentBounds);
    new (&right) BuildRange (target+N/2,N/2+N%2,rgeomBounds,rcentBounds);

    continuation(tid,data);
  }

  /*! explicit template instantiations */
  template class ObjectBinningParallel<2>;
}


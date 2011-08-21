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

#include "bvh2_builder_spatial.hpp"
#include "../common/compute_bounds.hpp"

namespace pf
{
  Ref<BVH2<Triangle4> > BVH2BuilderSpatial::build(const BuildTriangle* triangles, size_t numTriangles)
  {
    Ref<BVH2<Triangle4> > bvh = new BVH2<Triangle4>;
    double t0 = getSeconds();
    BVH2BuilderSpatial builder(triangles,numTriangles,bvh);
    double t1 = getSeconds();
    size_t bytesNodes = bvh->getNumNodes()*sizeof(BVH2<Triangle4>::Node);
    size_t bytesTris = bvh->getNumPrimBlocks()*sizeof(BVH2<Triangle4>::Triangle);
    std::cout <<
      "triangles = " << numTriangles << ", " <<
      "build time = " << (t1-t0)*1000.0f << "ms, " <<
      "sah = " << bvh->getSAH() << ", " <<
      "size = " << (bytesNodes+bytesTris)*1E-6 << " MB" << std::endl;
    std::cout <<
      "nodes = " << bvh->getNumNodes() << " "
              << "(" << bytesNodes*1E-6 << " MB) "
              << "(" << 100.0*(bvh->getNumNodes()-1+bvh->getNumLeaves())/(2.0*bvh->getNumNodes()     ) << "%), " <<
      "leaves = " << bvh->getNumLeaves() << " "
              << "(" << bytesTris*1E-6  << " MB) "
              << "(" << 100.0*bvh->getNumPrims()                        /(4.0*bvh->getNumPrimBlocks()) << "%), " <<
      "prims = " << bvh->getNumPrims() << " "
              << "(" << 100.0f*float(bvh->getNumPrims())/float(numTriangles) << "%)" << std::endl;
    return bvh;
  }

  const float BVH2BuilderSpatial::duplicationFactor = 1.5f;

  BVH2BuilderSpatial::BVH2BuilderSpatial(const BuildTriangle* triangles, size_t numTriangles, Ref<BVH2<Triangle4> > bvh)
    : numTriangles(numTriangles), triangles(triangles), bvh(bvh)
  {
    numThreads = scheduler->getNumThreads();

    /*! Allocate array for splitting primitive lists. */
    size_t maxTriangles = max((size_t)1,(size_t)(duplicationFactor*numTriangles));
    prims          = (Box*)alignedMalloc(maxTriangles*sizeof(Box));

    /*! Allocate space for job list */
    jobs = (SpatialBinning<2>*) alignedMalloc(maxTriangles*sizeof(SpatialBinning<2>));

    /*! Allocate storage for nodes. Each thread should at least be able to get one block. */
    allocatedNodes = 2*maxTriangles+numThreads*allocBlockSize;
    bvh->nodes     = (BVH2<Triangle4>::Node*)alignedMalloc(allocatedNodes*sizeof(BVH2<Triangle4>::Node));

    /*! Allocate storage for triangles. Each thread should at least be able to get one block. */
    allocatedPrimitives = maxTriangles+numThreads*allocBlockSize;
    bvh->triangles      = (Triangle4*)alignedMalloc(allocatedPrimitives*sizeof(Triangle4));

    /*! initiate parallel computation of bounds */
    ComputeBoundsTask computeBounds(triangles,numTriangles,prims);
    computeBounds.go();

    /*! start build */
    BuildRange range(0,numTriangles,computeBounds.geomBound,computeBounds.centBound);
    jobs[0] = SpatialBinning<2>(range,prims,triangles,1);
    jobs[0].node = &bvh->root;
    new BuildTaskHigh(this,0,maxTriangles,0,maxTriangles,1);
    scheduler->go();

    /*! rotate top part of tree */
    for (int i=0; i<5; i++) bvh->rotate(bvh->root,4);

    /*! free temporary memory again */
    bvh->nodes     = (BVH2<Triangle4>::Node*) alignedRealloc(bvh->nodes    ,atomicNextNode     *sizeof(BVH2<Triangle4>::Node));
    bvh->triangles = (Triangle4*            ) alignedRealloc(bvh->triangles,atomicNextPrimitive*sizeof(Triangle4            ));
    bvh->allocatedNodes     = atomicNextNode;
    bvh->allocatedTriangles = atomicNextPrimitive;
    alignedFree(prims); prims = NULL;
    alignedFree(jobs); jobs = NULL;
  }

  /***********************************************************************************************************************
   *                                         Breadth First Build Task
   **********************************************************************************************************************/

  INLINE BVH2BuilderSpatial::BuildTaskLow::BuildTaskLow(BVH2BuilderSpatial* parent,
                                                               size_t primBegin, size_t primEnd,
                                                               size_t jobBegin, size_t jobEnd, size_t numJobs)
    : parent(parent), tid(inf), primBegin(primBegin), primEnd(primEnd), jobBegin(jobBegin), jobEnd(jobEnd), numJobs(numJobs)
  {
    scheduler->addTask((Task::runFunction)&BuildTaskLow::run,this);
  }

  void BVH2BuilderSpatial::BuildTaskLow::run(size_t tid, BuildTaskLow* This, size_t elts)
  {
    /*! remember all roots in order to rotate the subtree later */
    size_t numRoots = min(size_t(128),This->numJobs);
    for (size_t i=0; i<numRoots; i++)
      This->roots[i] = This->parent->jobs[This->jobBegin+i].node;

    /*! build subtrees */
    This->tid = tid;
    This->build();

    /*! rotate subtrees */
    for (size_t i=0; i<numRoots; i++)
      This->parent->bvh->rotate(*This->roots[i],inf);

    delete This;
  }

  void BVH2BuilderSpatial::BuildTaskLow::build()
  {
    /*! counts number of generated triangles */
    size_t finished = 0;

    /*! continue until no jobs are left */
    for (bool l2r=true; numJobs!=0; l2r=!l2r)
    {
      size_t num = numJobs;
      size_t jobTarget  = l2r ? jobEnd  : jobBegin;
      size_t primTarget = l2r ? primEnd : primBegin;

      /*! process each split job */
      for (size_t i=0; i<num; i++)
      {
        size_t k = l2r ? jobBegin+num-i-1 : jobEnd-num+i;
        const SpatialBinning<2>& job = parent->jobs[k];
        bool spatial = job.spatialSAH < job.objectSAH && job.spatialSize+finished < (l2r ? primTarget-job.start() : job.end()-primTarget);

        /*! compute leaf and split cost */
        size_t N = job.size();
        float leafSAH  = BVH2<Triangle4>::intCost*job.leafSAH;
        float splitSAH = BVH2<Triangle4>::travCost*halfArea(job.geomBounds)+BVH2<Triangle4>::intCost*(spatial ? min(job.objectSAH,job.spatialSAH) : job.objectSAH);

        /*! make leaf node when threshold reached or SAH tells us */
        if (N <= 1 || (size_t)job.depth > BVH2<Triangle4>::maxDepth || (N <= BVH2<Triangle4>::maxLeafSize && leafSAH < splitSAH)) {
          *job.node = parent->bvh->createLeaf(parent->prims,parent->triangles,parent->threadAllocPrimitives(tid,blocks(N)),job.start(),N);
          finished += N;
          numJobs--;
          continue;
        }

        /*! get destination for left and right sub-job */
        if (l2r) jobTarget-=2;
        SpatialBinning<2>& left  = parent->jobs[jobTarget+0];
        SpatialBinning<2>& right = parent->jobs[jobTarget+1];
        if (!l2r) jobTarget+=2;

        /*! perform split */
        if (l2r) job.split_l2r(spatial,parent->prims,parent->triangles,left,right,primTarget);
        else     job.split_r2l(spatial,parent->prims,parent->triangles,left,right,primTarget);

        /*! create an inner node */
        int nodeID = (int)parent->threadAllocNodes(tid,1);
        BVH2<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
        node.set(0,left.geomBounds,0);  left.node = &node.child[0];
        node.set(1,right.geomBounds,0); right.node = &node.child[1];
        *job.node = BVH2<Triangle4>::id2offset(nodeID);
        numJobs++;
      }
    }
  }

  /***********************************************************************************************************************
   *                                            Breadth First Build Task
   **********************************************************************************************************************/

  INLINE BVH2BuilderSpatial::BuildTaskHigh::BuildTaskHigh(BVH2BuilderSpatial* parent,
                                                                 size_t primBegin, size_t primEnd,
                                                                 size_t jobBegin, size_t jobEnd, size_t numJobs)
    : parent(parent), primBegin(primBegin), primEnd(primEnd), jobBegin(jobBegin), jobEnd(jobEnd), numJobs(numJobs)
  {
    scheduler->addTask((Task::runFunction)&BuildTaskHigh::run,this);
  }

  void BVH2BuilderSpatial::BuildTaskHigh::run(size_t tid, BuildTaskHigh* This, size_t elts) {
    This->build();
    delete This;
  }

  void BVH2BuilderSpatial::BuildTaskHigh::build()
  {
    /*! counts number of generated triangles */
    size_t finished = 0;

    /*! continue until no jobs are left */
    for (bool l2r=true; numJobs!=0; l2r=!l2r)
    {
      size_t num = numJobs;
      size_t jobTarget  = l2r ? jobEnd  : jobBegin;
      size_t primTarget = l2r ? primEnd : primBegin;

      /*! process each split job */
      for (size_t i=0; i<num; i++)
      {
        size_t k = l2r ? jobBegin+num-i-1 : jobEnd-num+i;
        const SpatialBinning<2>& job = parent->jobs[k];
        bool spatial = job.spatialSAH < job.objectSAH && job.spatialSize+finished < (l2r ? primTarget-job.start() : job.end()-primTarget);

        /*! compute leaf and split cost */
        size_t N = job.size();
        float leafSAH  = BVH2<Triangle4>::intCost*job.leafSAH;
        float splitSAH = BVH2<Triangle4>::travCost*halfArea(job.geomBounds)+BVH2<Triangle4>::intCost*(spatial ? min(job.objectSAH,job.spatialSAH) : job.objectSAH);

        /*! make leaf node when threshold reached or SAH tells us */
        if (N <= 1 || (size_t)job.depth > BVH2<Triangle4>::maxDepth || (N <= BVH2<Triangle4>::maxLeafSize && leafSAH < splitSAH)) {
          *job.node = parent->bvh->createLeaf(parent->prims,parent->triangles,parent->globalAllocPrimitives(blocks(N)),job.start(),N);
          finished += N;
          numJobs--;
          continue;
        }

        /*! get destination for left and right sub-job */
        if (l2r) jobTarget-=2;
        SpatialBinning<2>& left  = parent->jobs[jobTarget+0];
        SpatialBinning<2>& right = parent->jobs[jobTarget+1];
        if (!l2r) jobTarget+=2;

        /*! perform split */
        if (l2r) job.split_l2r(spatial,parent->prims,parent->triangles,left,right,primTarget);
        else     job.split_r2l(spatial,parent->prims,parent->triangles,left,right,primTarget);

        /*! create an inner node */
        int nodeID = (int)parent->globalAllocNodes(1);
        BVH2<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
        node.set(0,left.geomBounds,0);  left.node = &node.child[0];
        node.set(1,right.geomBounds,0); right.node = &node.child[1];
        *job.node = BVH2<Triangle4>::id2offset(nodeID);
        numJobs++;
      }

      /*! Check if we can give jobs to other threads. We enter only
       *  when we just moved from left-to-right, thus we have to copy
       *  from right-to-left for allocating space to the subtasks. */
      if (l2r && numJobs > 2*parent->numThreads)
      {
        size_t firstJob = jobEnd-numJobs;
        size_t numThreads = parent->numThreads;
        size_t numPrimsRemaining = primEnd-parent->jobs[firstJob].start();
        size_t totalFreeSpace = primEnd-primBegin-numPrimsRemaining-finished;

        size_t subJobStart  = jobBegin;
        size_t subJobEnd    = jobBegin;
        size_t subPrimStart = primBegin;
        size_t subPrimEnd   = primBegin;

        for (size_t i=firstJob; i<jobEnd; i++)
        {
          /*! copy job to the left */
          const SpatialBinning<2>& job = parent->jobs[i];
          size_t jobSize = job.size();
          parent->jobs[subJobEnd] = job;
          parent->jobs[subJobEnd++].moveTo(subPrimEnd);
          memmove(&parent->prims[subPrimEnd],&parent->prims[job.start()],jobSize*sizeof(Box));
          subPrimEnd += jobSize;

          /*! create subtask if enough primitives collected */
          size_t numPrims = subPrimEnd - subPrimStart;
          if (numPrims > float(numPrimsRemaining)/float(numThreads) || i+1 == jobEnd)
          {
            /*! distribute remaining free space */
            size_t taskFreeSpace = min(totalFreeSpace,size_t(float(numPrims)/float(numPrimsRemaining)*totalFreeSpace));
            subPrimEnd     += taskFreeSpace;
            totalFreeSpace -= taskFreeSpace;
            size_t numJobs = subJobEnd-subJobStart;
            subJobEnd = subJobStart+numPrims+taskFreeSpace;   // we need enough space for split jobs
            assert(subJobEnd <= jobEnd);

            /*! create sub-task */
            new BuildTaskLow(parent,subPrimStart,subPrimEnd,subJobStart,subJobEnd,numJobs);
            subJobStart = subJobEnd;
            subPrimStart = subPrimEnd;
          }
          numPrimsRemaining -= jobSize; // do not use job.size() as subtask might run already!
        }
        numJobs = 0; //!< we finish
      }
    }
  }
}


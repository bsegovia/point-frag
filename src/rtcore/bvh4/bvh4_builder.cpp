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

#include "bvh4_builder.hpp"
#include "../common/compute_bounds.hpp"

namespace pf
{
  Ref<BVH4<Triangle4> > BVH4Builder::build(const BuildTriangle* triangles, size_t numTriangles)
  {
    Ref<BVH4<Triangle4> > bvh = new BVH4<Triangle4>;
    double t0 = getSeconds();
    BVH4Builder builder(triangles,numTriangles,bvh);
    double t1 = getSeconds();
    size_t bytesNodes = bvh->getNumNodes()*sizeof(BVH4<Triangle4>::Node);
    size_t bytesTris = bvh->getNumPrimBlocks()*sizeof(BVH4<Triangle4>::Triangle);
    std::cout <<
      "triangles = " << numTriangles << ", " <<
      "build time = " << (t1-t0)*1000.0f << "ms, " <<
      "sah = " << bvh->getSAH() << ", " <<
      "size = " << (bytesNodes+bytesTris)*1E-6 << " MB" << std::endl;
    std::cout <<
      "nodes = "  << bvh->getNumNodes()  << " (" << bytesNodes*1E-6 << " MB) (" << 100.0*(bvh->getNumNodes()-1+bvh->getNumLeaves())/(4.0*bvh->getNumNodes()     ) << "%), " <<
      "leaves = " << bvh->getNumLeaves() << " (" << bytesTris*1E-6  << " MB) (" << 100.0*bvh->getNumPrims()                        /(4.0*bvh->getNumPrimBlocks()) << "%)" << std::endl;
    return bvh;
  }

  BVH4Builder::BVH4Builder(const BuildTriangle* triangles, size_t numTriangles, Ref<BVH4<Triangle4> > bvh)
    : triangles(triangles), numTriangles(numTriangles), bvh(bvh)
  {
    size_t numThreads = scheduler->getNumThreads();

    /*! Allocate storage for nodes. Each thread should at least be able to get one block. */
    allocatedNodes = numTriangles+numThreads*allocBlockSize;
    bvh->nodes = (BVH4<Triangle4>::Node*)alignedMalloc(allocatedNodes*sizeof(BVH4<Triangle4>::Node));

    /*! Allocate storage for triangles. Each thread should at least be able to get one block. */
    allocatedPrimitives = numTriangles+numThreads*allocBlockSize;
    bvh->triangles = (Triangle4*)alignedMalloc(allocatedPrimitives*sizeof(Triangle4));

    /*! Allocate array for splitting primitive lists. 2*N required for parallel splits. */
    prims = (Box*)alignedMalloc(2*numTriangles*sizeof(Box));

    /*! initiate parallel computation of bounds */
    ComputeBoundsTask computeBounds(triangles,numTriangles,prims);
    computeBounds.go();

    /*! start build */
    recurse(bvh->root,1,BuildRange(0,numTriangles,computeBounds.geomBound,computeBounds.centBound));
    scheduler->go();

    /*! rotate top part of tree */
    for (int i=0; i<5; i++) bvh->rotate(bvh->root,4);

    /*! free temporary memory again */
    bvh->nodes     = (BVH4<Triangle4>::Node*) alignedRealloc(bvh->nodes    ,atomicNextNode     *sizeof(BVH4<Triangle4>::Node));
    bvh->triangles = (Triangle4*            ) alignedRealloc(bvh->triangles,atomicNextPrimitive*sizeof(Triangle4            ));
    alignedFree(prims); prims = NULL;
  }

  void BVH4Builder::recurse(int& nodeID, size_t depth, const BuildRange& job)
  {
    /*! use full single threaded build for small jobs */
    if (job.size() < 4*1024) new BuildTask(this,nodeID,depth,ObjectBinning<2>(job,prims));

    /*! use single threaded split for medium size jobs  */
    else if (job.size() < 2048*1024) new SplitTask(this,nodeID,depth,ObjectBinning<2>(job,prims));

    /*! use parallel splitter for big jobs */
    else new ParallelSplitTask(this,nodeID,depth,job);
  }

  void BVH4Builder::recurse(int& nodeID, size_t depth, const ObjectBinning<2>& job)
  {
    /*! use full single threaded build for small jobs */
    if (job.size() < 4*1024) new BuildTask(this,nodeID,depth,job);

    /*! use single theaded split task to create subjobs */
    else new SplitTask(this,nodeID,depth,job);
  }

  /***********************************************************************************************************************
   *                                         Full Recursive Build Task
   **********************************************************************************************************************/

  INLINE BVH4Builder::BuildTask::BuildTask(BVH4Builder* parent, int& nodeID, size_t depth, const ObjectBinning<2>& job)
    : parent(parent), tid(inf), nodeID(nodeID), depth(depth), job(job)
  {
    scheduler->addTask((Task::runFunction)&BuildTask::run,this);
  }

  void BVH4Builder::BuildTask::run(size_t tid, BuildTask* This, size_t elts)
  {
    This->tid = tid;
    This->nodeID = This->recurse(This->depth,This->job);
    for (int i=0; i<5; i++) This->parent->bvh->rotate(This->nodeID,inf);
    delete This;
  }

  int BVH4Builder::BuildTask::recurse(size_t depth, ObjectBinning<2>& job)
  {
    /*! compute leaf and split cost */
    size_t N = job.size();
    float leafSAH  = BVH4<Triangle4>::intCost*job.leafSAH;
    float splitSAH = BVH4<Triangle4>::travCost*halfArea(job.geomBounds)+BVH4<Triangle4>::intCost*job.splitSAH;

    /*! make leaf node when threshold reached or SAH tells us */
    if (N <= 1 || depth > BVH4<Triangle4>::maxDepth || (N <= BVH4<Triangle4>::maxLeafSize && leafSAH < splitSAH))
      return parent->bvh->createLeaf(parent->prims,parent->triangles,parent->threadAllocPrimitives(tid,blocks(N)),job.start(),N);

    /*! perform initial split */
    size_t numChildren = 2;
    ObjectBinning<2> children[4];
    job.split(parent->prims,children[0],children[1]);

    /*! split until node is full or SAH tells us to stop */
    do {

      /*! find best child to split */
      float bestSAH = 0;
      index_t bestChild = -1;
      for (size_t i=0; i<numChildren; i++) {
        float dSAH = children[i].splitSAH-children[i].leafSAH;
        if (children[i].size() > BVH4<Triangle4>::maxLeafSize) dSAH = min(0.0f,dSAH); //< force split for large jobs
        if (dSAH <= bestSAH) { bestChild = i; bestSAH = dSAH; }
      }
      if (bestChild == -1) break;

      /*! perform split of best possible child */
      children[bestChild].split(parent->prims,children[bestChild],children[numChildren++]);

    } while (numChildren < 4);

    /*! create an inner node */
    int nodeID = (int)parent->threadAllocNodes(tid,1);
    BVH4<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
    for (size_t i=0; i<numChildren; i++) node.set(i,children[i].geomBounds,recurse(depth+1,children[i]));
    return BVH4<Triangle4>::id2offset(nodeID);
  }

  /***********************************************************************************************************************
   *                                      Single Threaded Split Task
   **********************************************************************************************************************/

  INLINE BVH4Builder::SplitTask::SplitTask(BVH4Builder* parent, int& nodeID, size_t depth, const ObjectBinning<2>& job)
    : parent(parent), nodeID(nodeID), depth(depth), job(job)
  {
    scheduler->addTask((Task::runFunction)_split,this);
  }

  void BVH4Builder::SplitTask::split()
  {
    /*! compute leaf and split cost */
    size_t N = job.size();
    float leafSAH = BVH4<Triangle4>::intCost*job.leafSAH;
    float splitSAH = BVH4<Triangle4>::travCost*halfArea(job.geomBounds)+BVH4<Triangle4>::intCost*job.splitSAH;

    /*! make leaf node when threshold reached or SAH tells us */
    if (N <= 1 || depth > BVH4<Triangle4>::maxDepth || (N <= BVH4<Triangle4>::maxLeafSize && leafSAH < splitSAH)) {
      this->nodeID = parent->bvh->createLeaf(parent->prims,parent->triangles,parent->globalAllocPrimitives(blocks(N)),job.start(),N);
      delete this;
      return;
    }

    /*! perform initial split */
    size_t numChildren = 2;
    ObjectBinning<2> children[4];
    job.split(parent->prims,children[0],children[1]);

    /*! split until node is full or SAH tells us to stop */
    do {

      /*! find best child to split */
      float bestSAH = 0;
      index_t bestChild = -1;
      for (size_t i=0; i<numChildren; i++) {
        float dSAH = children[i].splitSAH-children[i].leafSAH;
        if (children[i].size() > BVH4<Triangle4>::maxLeafSize) dSAH = min(0.0f,dSAH); //< force split for large jobs
        if (dSAH < bestSAH) { bestChild = i; bestSAH = dSAH; }
      }
      if (bestChild == -1) break;

      /*! perform split of best possible child */
      children[bestChild].split(parent->prims,children[bestChild],children[numChildren++]);

    } while (numChildren < 4);

    /*! create an inner node */
    int nodeID = (int)parent->globalAllocNodes(1);
    this->nodeID = BVH4<Triangle4>::id2offset(nodeID);
    BVH4<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
    for (size_t i=0; i<numChildren; i++) {
      node.set(i,children[i].geomBounds,0);
      parent->recurse(node.child[i],depth+1,children[i]);
    }
    delete this;
  }

  /***********************************************************************************************************************
   *                                           Parallel Split Task
   **********************************************************************************************************************/

  INLINE BVH4Builder::ParallelSplitTask::ParallelSplitTask(BVH4Builder* parent, int& nodeID, size_t depth, const BuildRange& job)
    : parent(parent), nodeID(nodeID), depth(depth), numChildren(1), bestChild(0)
  {
    new (&children[0]) ObjectBinningParallel<2>(job,target(job),parent->prims);
    children[0].go((Task::completeFunction)_stage0,this);
  }

  void BVH4Builder::ParallelSplitTask::stage0(size_t tid)
  {
    /*! compute leaf and split cost */
    ObjectBinningParallel<2>& job = children[0];
    size_t N = job.size();
    float leafSAH = BVH4<Triangle4>::intCost*job.leafSAH;
    float splitSAH = BVH4<Triangle4>::travCost*halfArea(job.geomBounds)+BVH4<Triangle4>::intCost*job.splitSAH;

    /*! make leaf node when threshold reached or SAH tells us */
    if (N <= 1 || depth > BVH4<Triangle4>::maxDepth || (N <= BVH4<Triangle4>::maxLeafSize && leafSAH < splitSAH)) {
      this->nodeID = parent->bvh->createLeaf(parent->prims,parent->triangles,parent->globalAllocPrimitives(blocks(N)),job.start(),N);
      delete this;
      return;
    }

    /*! goto next stage */
    stage1(tid);
  }

  void BVH4Builder::ParallelSplitTask::stage1(size_t tid)
  {
    new (&children[numChildren]) ObjectBinningParallel<2>(children[bestChild].right,target(children[bestChild].right),parent->prims);
    children[numChildren++].go((Task::completeFunction)_stage2,this);
  }

  void BVH4Builder::ParallelSplitTask::stage2(size_t tid)
  {
    new (&children[bestChild]) ObjectBinningParallel<2>(children[bestChild].left,target(children[bestChild].left),parent->prims);
    children[bestChild].go((Task::completeFunction)_stage3,this);
  }

  void BVH4Builder::ParallelSplitTask::stage3(size_t tid)
  {
    /*! continue splitting as long as node not full */
    if (numChildren < 4)
    {
      /*! find best child to split next */
      bestChild = -1;
      float bestSAH = 0;
      for (size_t i=0; i<numChildren; i++) {
        float dSAH = children[i].splitSAH-children[i].leafSAH;
        if (children[i].size() > BVH4<Triangle4>::maxLeafSize) dSAH = min(0.0f,dSAH); //< force split for large jobs
        if (dSAH < bestSAH) { bestChild = i; bestSAH = dSAH; }
      }
      if (bestChild >= 0)
        return stage1(tid);
    }

    /*! create an inner node */
    int nodeID = (int)parent->globalAllocNodes(1);
    this->nodeID = BVH4<Triangle4>::id2offset(nodeID);
    BVH4<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
    for (size_t i=0; i<numChildren; i++) {
      node.set(i,children[i].geomBounds,0);
      parent->recurse(node.child[i],depth+1,children[i]);
    }
    delete this;
  }
}



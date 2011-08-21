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

#include "bvh2_builder.hpp"
#include "../common/compute_bounds.hpp"

namespace pf
{
  Ref<BVH2<Triangle4> > BVH2Builder::build(const BuildTriangle* triangles, size_t numTriangles)
  {
    Ref<BVH2<Triangle4> > bvh = new BVH2<Triangle4>;
    double t0 = getSeconds();
    BVH2Builder builder(triangles,numTriangles,bvh);
    double t1 = getSeconds();
    size_t bytesNodes = bvh->getNumNodes()*sizeof(BVH2<Triangle4>::Node);
    size_t bytesTris = bvh->getNumPrimBlocks()*sizeof(BVH2<Triangle4>::Triangle);
    std::cout <<
      "triangles = " << numTriangles << ", " <<
      "build time = " << (t1-t0)*1000.0f << "ms, " <<
      "sah = " << bvh->getSAH() << ", " <<
      "size = " << (bytesNodes+bytesTris)*1E-6 << " MB" << std::endl;
    std::cout <<
      "nodes = "  << bvh->getNumNodes()  << " (" << bytesNodes*1E-6 << " MB) (" << 100.0*(bvh->getNumNodes()-1+bvh->getNumLeaves())/(2.0*bvh->getNumNodes()     ) << "%), " <<
      "leaves = " << bvh->getNumLeaves() << " (" << bytesTris*1E-6  << " MB) (" << 100.0*bvh->getNumPrims()                        /(4.0*bvh->getNumPrimBlocks()) << "%)" << std::endl;
    return bvh;
  }

  BVH2Builder::BVH2Builder(const BuildTriangle* triangles, size_t numTriangles, Ref<BVH2<Triangle4> > bvh)
    : triangles(triangles), numTriangles(numTriangles), bvh(bvh)
  {
    size_t numThreads = scheduler->getNumThreads();

    /*! Allocate storage for nodes. Each thread should at least be able to get one block. */
    allocatedNodes = numTriangles+numThreads*allocBlockSize;
    bvh->nodes = (BVH2<Triangle4>::Node*)alignedMalloc(allocatedNodes*sizeof(BVH2<Triangle4>::Node));

    /*! Allocate storage for triangles. Each thread should at least be able to get one block. */
    allocatedPrimitives = numTriangles+numThreads*allocBlockSize;
    bvh->triangles      = (Triangle4*)alignedMalloc(allocatedPrimitives*sizeof(Triangle4));

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
    bvh->nodes     = (BVH2<Triangle4>::Node*) alignedRealloc(bvh->nodes    ,atomicNextNode     *sizeof(BVH2<Triangle4>::Node));
    bvh->triangles = (Triangle4*            ) alignedRealloc(bvh->triangles,atomicNextPrimitive*sizeof(Triangle4            ));
    bvh->allocatedNodes     = atomicNextNode;
    bvh->allocatedTriangles = atomicNextPrimitive;
    alignedFree(prims); prims = NULL;
  }

  void BVH2Builder::recurse(int& nodeID, size_t depth, const BuildRange& job)
  {
    /*! use full single threaded build for small jobs */
    if (job.size() < 4*1024)
      new BuildTask(this,nodeID,depth,ObjectBinning<2>(job,prims));

    /*! use single threaded split for medium size jobs  */
    else if (job.size() < 2048*1024) new SplitTask(this,nodeID,depth,ObjectBinning<2>(job,prims));

    /*! use parallel splitter for big jobs */
    else new ParallelSplitTask(this,nodeID,depth,job);
  }

  void BVH2Builder::recurse(int& nodeID, size_t depth, const ObjectBinning<2>& job)
  {
    /*! use full single threaded build for small jobs */
    if (job.size() < 4*1024) new BuildTask(this,nodeID,depth,job);

    /*! use single theaded split task to create subjobs */
    else new SplitTask(this,nodeID,depth,job);
  }

  /***********************************************************************************************************************
   *                                         Full Recursive Build Task
   **********************************************************************************************************************/

  INLINE BVH2Builder::BuildTask::BuildTask(BVH2Builder* parent, int& nodeID, size_t depth, const ObjectBinning<2>& job)
    : parent(parent), tid(inf), nodeID(nodeID), depth(depth), job(job)
  {
    scheduler->addTask((Task::runFunction)&BuildTask::run,this);
  }

  void BVH2Builder::BuildTask::run(size_t tid, BuildTask* This, size_t elts)
  {
    This->tid = tid;
    This->nodeID = This->recurse(This->depth,This->job);
    for (int i=0; i<5; i++) This->parent->bvh->rotate(This->nodeID,inf);
    delete This;
  }

  int BVH2Builder::BuildTask::recurse(size_t depth, ObjectBinning<2>& job)
  {
    /*! compute leaf and split cost */
    size_t N = job.size();
    float leafSAH  = BVH2<Triangle4>::intCost*job.leafSAH;
    float splitSAH = BVH2<Triangle4>::travCost*halfArea(job.geomBounds)+BVH2<Triangle4>::intCost*job.splitSAH;

    /*! make leaf node when threshold reached or SAH tells us */
    if (N <= 1 || depth > BVH2<Triangle4>::maxDepth || (N <= BVH2<Triangle4>::maxLeafSize && leafSAH < splitSAH))
      return parent->bvh->createLeaf(parent->prims,parent->triangles,parent->threadAllocPrimitives(tid,blocks(N)),job.start(),N);

    /*! perform split */
    ObjectBinning<2> left,right;
    job.split(parent->prims,left,right);

    /*! create an inner node */
    int nodeID = (int)parent->threadAllocNodes(tid,1);
    BVH2<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
    node.set(0,left .geomBounds,recurse(depth+1,left ));
    node.set(1,right.geomBounds,recurse(depth+1,right));
    return BVH2<Triangle4>::id2offset(nodeID);
  }

  /***********************************************************************************************************************
   *                                      Single Threaded Split Task
   **********************************************************************************************************************/

  INLINE BVH2Builder::SplitTask::SplitTask(BVH2Builder* parent, int& nodeID, size_t depth, const ObjectBinning<2>& job)
    : parent(parent), nodeID(nodeID), depth(depth), job(job)
  {
    scheduler->addTask((Task::runFunction)_split,this);
  }

  void BVH2Builder::SplitTask::split()
  {
    /*! compute leaf and split cost */
    size_t N = job.size();
    float leafSAH = BVH2<Triangle4>::intCost*job.leafSAH;
    float splitSAH = BVH2<Triangle4>::travCost*halfArea(job.geomBounds)+BVH2<Triangle4>::intCost*job.splitSAH;

    /*! make leaf node when threshold reached or SAH tells us */
    if (N <= 1 || depth > BVH2<Triangle4>::maxDepth || (N <= BVH2<Triangle4>::maxLeafSize && leafSAH < splitSAH)) {
      this->nodeID = parent->bvh->createLeaf(parent->prims,parent->triangles,parent->globalAllocPrimitives(blocks(N)),job.start(),N);
      delete this;
      return;
    }

    /*! perform split */
    ObjectBinning<2> left,right;
    job.split(parent->prims,left,right);

    /*! create an inner node */
    int nodeID = (int)parent->globalAllocNodes(1);
    this->nodeID = BVH2<Triangle4>::id2offset(nodeID);
    BVH2<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
    node.set(0,left .geomBounds,0);
    node.set(1,right.geomBounds,0);
    parent->recurse(node.child[0],depth+1,left );
    parent->recurse(node.child[1],depth+1,right);
    delete this;
  }

  /***********************************************************************************************************************
   *                                           Parallel Split Task
   **********************************************************************************************************************/

  INLINE BVH2Builder::ParallelSplitTask::ParallelSplitTask(BVH2Builder* parent, int& nodeID, size_t depth, const BuildRange& job)
    : parent(parent), nodeID(nodeID), depth(depth)
  {
    new (&binner) ObjectBinningParallel<2>(job,target(job),parent->prims);
    binner.go((Task::completeFunction)_createNode,this);
  }

  void BVH2Builder::ParallelSplitTask::createNode(size_t tid)
  {
    /*! compute leaf and split cost */
    size_t N = binner.size();
    float leafSAH = BVH2<Triangle4>::intCost*binner.leafSAH;
    float splitSAH = BVH2<Triangle4>::travCost*halfArea(binner.geomBounds)+BVH2<Triangle4>::intCost*binner.splitSAH;

    /*! make leaf node when threshold reached or SAH tells us */
    if (N <= 1 || depth > BVH2<Triangle4>::maxDepth || (N <= BVH2<Triangle4>::maxLeafSize && leafSAH < splitSAH)) {
      this->nodeID = parent->bvh->createLeaf(parent->prims,parent->triangles,parent->globalAllocPrimitives(blocks(N)),binner.start(),N);
      delete this;
      return;
    }

    /*! create an inner node */
    int nodeID = (int)parent->globalAllocNodes(1);
    this->nodeID = BVH2<Triangle4>::id2offset(nodeID);
    BVH2<Triangle4>::Node& node = parent->bvh->nodes[nodeID].clear();
    node.set(0,binner.left .geomBounds,0);
    node.set(1,binner.right.geomBounds,0);
    parent->recurse(node.child[0],depth+1,binner.left );
    parent->recurse(node.child[1],depth+1,binner.right);
    delete this;
  }
}


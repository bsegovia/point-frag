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

#include "bvh2_to_bvh4.hpp"

namespace pf
{
  Ref<BVH4<Triangle4> > BVH2ToBVH4::convert(Ref<BVH2<Triangle4> >& bvh2)
  {
    Ref<BVH4<Triangle4> > bvh4 = new BVH4<Triangle4>;
    double t0 = getSeconds();
    BVH2ToBVH4 builder(bvh2,bvh4);
    double t1 = getSeconds();
    size_t bytesNodes = bvh4->getNumNodes()*sizeof(BVH4<Triangle4>::Node);
    size_t bytesTris = bvh4->getNumPrimBlocks()*sizeof(BVH4<Triangle4>::Triangle);
    std::cout <<
      "build time = " << (t1-t0)*1000.0f << "ms, " <<
      "sah = " << bvh4->getSAH() << ", " <<
      "size = " << (bytesNodes+bytesTris)*1E-6 << " MB" << std::endl;
    std::cout <<
      "nodes = "  << bvh4->getNumNodes()  << " (" << bytesNodes*1E-6 << " MB) (" << 100.0*(bvh4->getNumNodes()-1+bvh4->getNumLeaves())/(4.0*bvh4->getNumNodes()     ) << "%), " <<
      "leaves = " << bvh4->getNumLeaves() << " (" << bytesTris*1E-6  << " MB) (" << 100.0*bvh4->getNumPrims()                         /(4.0*bvh4->getNumPrimBlocks()) << "%)" << std::endl;
    return bvh4;
  }

  BVH2ToBVH4::BVH2ToBVH4(Ref<BVH2<Triangle4> >& bvh2, Ref<BVH4<Triangle4> >& bvh4)
    : bvh2(bvh2), bvh4(bvh4)
  {
    /*! Allocate storage for nodes. Each thread should at least be able to get one block. */
    allocatedNodes = bvh2->allocatedNodes;
    bvh4->nodes = (BVH4<Triangle4>::Node*)alignedMalloc(allocatedNodes*sizeof(BVH4<Triangle4>::Node));

    /*! recursively convert tree */
    bvh4->root = recurse(bvh2->root,1);

    /*! rotate tree */
    bvh4->rotate(bvh4->root,inf);

    /*! resize node array and assign triangles */
    bvh4->nodes     = (BVH4<Triangle4>::Node*) alignedRealloc(bvh4->nodes,atomicNextNode*sizeof(BVH4<Triangle4>::Node));
    bvh4->triangles = bvh2->triangles;
    bvh2->triangles = NULL;
  }

  /*! recursively converts BVH2 into BVH4 */
  int BVH2ToBVH4::recurse(int parent, int depth)
  {
    /*! do not touch leaf nodes */
    if (parent < 0) return parent;

    /*! initialize first two nodes */
    int nodes[4];
    Box bounds[4];
    size_t numChildren = 2;
    nodes [0] = bvh2->node(parent).child[0];
    bounds[0] = bvh2->node(parent).bounds(0);
    nodes [1] = bvh2->node(parent).child[1];
    bounds[1] = bvh2->node(parent).bounds(1);

    /*! find the node with largest bounding box */
    while (numChildren < 4)
    {
      index_t bestIdx = -1;
      float bestArea = neg_inf;
      for (size_t i=0; i<numChildren; i++)
      {
        /*! skip leaf nodes */
        if (nodes[i] < 0) continue;

        /*! find largest bounding */
        float A = halfArea(bounds[i]);
        if (A > bestArea) {
          bestArea = A;
          bestIdx = i;
        }
      }
      if (bestIdx<0) break;

      /*! recurse into best candidate */
      int bestNode = nodes[bestIdx];
      nodes [bestIdx]     = bvh2->node(bestNode).child[0];
      bounds[bestIdx]     = bvh2->node(bestNode).bounds(0);
      nodes [numChildren] = bvh2->node(bestNode).child[1];
      bounds[numChildren] = bvh2->node(bestNode).bounds(1);
      numChildren++;
    }

    /*! encode BVH4 node and recurse */
    int nodeID = (int)globalAllocNodes(1);
    BVH4<Triangle4>::Node& node = bvh4->nodes[nodeID].clear();
    for (size_t i=0; i<numChildren; i++) node.set(i,bounds[i],recurse(nodes[i],depth+1));
    return BVH4<Triangle4>::id2offset(nodeID);
  }
}



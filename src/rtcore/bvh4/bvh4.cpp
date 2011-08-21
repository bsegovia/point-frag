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

#include "bvh4.hpp"
#include "triangle4.hpp"

namespace pf
{
  template<typename T>
  int BVH4<T>::createLeaf(const Box* prims, const BuildTriangle* triangles_i, size_t nextTriangle, size_t start, size_t N)
  {
    if (nextTriangle >= (1<<26)) throw std::runtime_error("cannot encode triangle, ID too large");

    /*! In these SSE vectors we gather blocks of 4 triangles. */
    sse3f v0 = zero, v1 = zero, v2 = zero;
    ssei id0 = -1, id1 = -1;

    size_t slot = 0, numBlocks = 0;
    for (size_t i=0; i<N; i++)
    {
      int id = prims[start+i].lower.i[3];
      const BuildTriangle& tri = triangles_i[id];
      id0 [slot] = tri.id0;
      id1 [slot] = tri.id1;
      v0.x[slot] = tri.x0; v0.y[slot] = tri.y0; v0.z[slot] = tri.z0;
      v1.x[slot] = tri.x1; v1.y[slot] = tri.y1; v1.z[slot] = tri.z1;
      v2.x[slot] = tri.x2; v2.y[slot] = tri.y2; v2.z[slot] = tri.z2;
      slot++;

      if (slot == 4 || i+1==N)
      {
        triangles[nextTriangle++] = Triangle(v0,v1,v2,id0,id1);
        id0 = -1; id1 = -1;
        v0 = zero; v1 = zero; v2 = zero;
        slot = 0; numBlocks++;
      }
    }

    if (numBlocks > maxLeafSize)
      std::cout << "BVH4: loosing triangles" << std::endl;

    return int(emptyNode) | 32*int(nextTriangle-numBlocks) | int(min(numBlocks,size_t(maxLeafSize)));
  }

  template<typename T>
  void BVH4<T>::rotate(int nodeID, int maxDepth)
  {
    /*! nothing to rotate if we reached a leaf node. */
    if (nodeID < 0 || maxDepth < 0) return;
    Node& parent = node(nodeID);

    /*! rotate all children first */
    for (size_t c=0; c<4; c++)
      rotate(parent.child[c],maxDepth-1);

    /* compute current area of all children */
    ssef sizeX = parent.upper_x-parent.lower_x, sizeY = parent.upper_y-parent.lower_y, sizeZ = parent.upper_z-parent.lower_z;
    ssef childArea = sizeX*(sizeY + sizeZ) + sizeY*sizeZ;

    /*! transpose node bounds */
    ssef plower0,plower1,plower2,plower3; transpose(parent.lower_x,parent.lower_y,parent.lower_z,ssef(zero),plower0,plower1,plower2,plower3);
    ssef pupper0,pupper1,pupper2,pupper3; transpose(parent.upper_x,parent.upper_y,parent.upper_z,ssef(zero),pupper0,pupper1,pupper2,pupper3);
    Box other0(plower0,pupper0), other1(plower1,pupper1), other2(plower2,pupper2), other3(plower3,pupper3);

    /*! Find best rotation. We pick a target child of a first child,
      and swap this with an other child. We perform the best such
      swap. */
    float bestCost = pos_inf;
    int bestChild = -1, bestTarget = -1, bestOther = -1;
    for (size_t c=0; c<4; c++)
    {
      /*! ignore leaf nodes as we cannot descent into */
      if (parent.child[c] < 0) continue;
      Node& child = node(parent.child[c]);

      /*! transpose child bounds */
      ssef clower0,clower1,clower2,clower3; transpose(child.lower_x,child.lower_y,child.lower_z,ssef(zero),clower0,clower1,clower2,clower3);
      ssef cupper0,cupper1,cupper2,cupper3; transpose(child.upper_x,child.upper_y,child.upper_z,ssef(zero),cupper0,cupper1,cupper2,cupper3);
      Box target0(clower0,cupper0), target1(clower1,cupper1), target2(clower2,cupper2), target3(clower3,cupper3);

      /*! put other0 at each target position */
      float cost00 = halfArea(merge(other0 ,target1,target2,target3));
      float cost01 = halfArea(merge(target0,other0 ,target2,target3));
      float cost02 = halfArea(merge(target0,target1,other0 ,target3));
      float cost03 = halfArea(merge(target0,target1,target2,other0 ));
      ssef cost0 = ssef(cost00,cost01,cost02,cost03);
      ssef min0 = reduce_min(cost0);
      int pos0 = (int)__bsf(movemask(min0 == cost0));

      /*! put other1 at each target position */
      float cost10 = halfArea(merge(other1 ,target1,target2,target3));
      float cost11 = halfArea(merge(target0,other1 ,target2,target3));
      float cost12 = halfArea(merge(target0,target1,other1 ,target3));
      float cost13 = halfArea(merge(target0,target1,target2,other1 ));
      ssef cost1 = ssef(cost10,cost11,cost12,cost13);
      ssef min1 = reduce_min(cost1);
      int pos1 = (int)__bsf(movemask(min1 == cost1));

      /*! put other2 at each target position */
      float cost20 = halfArea(merge(other2 ,target1,target2,target3));
      float cost21 = halfArea(merge(target0,other2 ,target2,target3));
      float cost22 = halfArea(merge(target0,target1,other2 ,target3));
      float cost23 = halfArea(merge(target0,target1,target2,other2 ));
      ssef cost2 = ssef(cost20,cost21,cost22,cost23);
      ssef min2 = reduce_min(cost2);
      int pos2 = (int)__bsf(movemask(min2 == cost2));

      /*! put other3 at each target position */
      float cost30 = halfArea(merge(other3 ,target1,target2,target3));
      float cost31 = halfArea(merge(target0,other3 ,target2,target3));
      float cost32 = halfArea(merge(target0,target1,other3 ,target3));
      float cost33 = halfArea(merge(target0,target1,target2,other3 ));
      ssef cost3 = ssef(cost30,cost31,cost32,cost33);
      ssef min3 = reduce_min(cost3);
      int pos3 = (int)__bsf(movemask(min3 == cost3));

      /*! find best other child */
      ssef otherCost = ssef(extract<0>(min0),extract<0>(min1),extract<0>(min2),extract<0>(min3));
      int pos[4] = { pos0,pos1,pos2,pos3 };
      ssef bestOtherCost = min(min0,min1,min2,min3);
      size_t n = __bsf(movemask(bestOtherCost == otherCost));
      float cost = extract<0>(bestOtherCost)-childArea[c]; //< increasing the original child bound is bad, decreasing good

      /*! accept a swap when it reduces cost and is not swapping a node with itself */
      if (cost < bestCost && n != c) {
        bestCost = cost;
        bestChild = (int)c;
        bestOther = (int)n;
        bestTarget = pos[n];
      }
    }

    /*! if we did not find a swap that improves the SAH then do nothing */
    if (bestCost >= 0) return;

    /*! perform the best found tree rotation */
    Node& child = node(parent.child[bestChild]);
    std::swap(parent.child  [bestOther],child.child  [bestTarget]);
    std::swap(parent.lower_x[bestOther],child.lower_x[bestTarget]);
    std::swap(parent.lower_y[bestOther],child.lower_y[bestTarget]);
    std::swap(parent.lower_z[bestOther],child.lower_z[bestTarget]);
    std::swap(parent.upper_x[bestOther],child.upper_x[bestTarget]);
    std::swap(parent.upper_y[bestOther],child.upper_y[bestTarget]);
    std::swap(parent.upper_z[bestOther],child.upper_z[bestTarget]);
    parent.lower_x[bestChild] = extract<0>(reduce_min(child.lower_x));
    parent.lower_y[bestChild] = extract<0>(reduce_min(child.lower_y));
    parent.lower_z[bestChild] = extract<0>(reduce_min(child.lower_z));
    parent.upper_x[bestChild] = extract<0>(reduce_max(child.upper_x));
    parent.upper_y[bestChild] = extract<0>(reduce_max(child.upper_y));
    parent.upper_z[bestChild] = extract<0>(reduce_max(child.upper_z));
  }

  template<typename T>
  void BVH4<T>::computeStatistics()
  {
    if (modified) {
      numNodes = 0;
      numLeaves = 0;
      numPrimBlocks = 0;
      numPrims = 0;
      bvhSAH = computeStatistics(root,0.0f);
      modified = false;
    }
  }

  template<typename T>
  float BVH4<T>::computeStatistics(int nodeID, float ap)
  {
    if (nodeID >= 0)
    {
      numNodes++;
      const Node& n = node(nodeID);
      ssef ac = 2.0f*((n.upper_x-n.lower_x)*(n.upper_y-n.lower_y+n.upper_z-n.lower_z)+(n.upper_y-n.lower_y)*(n.upper_z-n.lower_z));
      float sah0 = computeStatistics(n.child[0],ac[0]);
      float sah1 = computeStatistics(n.child[1],ac[1]);
      float sah2 = computeStatistics(n.child[2],ac[2]);
      float sah3 = computeStatistics(n.child[3],ac[3]);
      return ap*travCost + sah0 + sah1 + sah2 + sah3;
    }
    else
    {
      nodeID ^= 0x80000000;
      size_t ofs = size_t(nodeID) >> 5;
      size_t num = size_t(nodeID) & 0x1F;
      if (!num) return 0.0f;

      numLeaves++;
      numPrimBlocks += num;
      for (size_t i=ofs; i<ofs+num; i++)
        numPrims += triangles[i].size();
      return intCost * ap * num;
    }
  }

  /*! explicit template instantiations */
  template class BVH4<Triangle4>;
}

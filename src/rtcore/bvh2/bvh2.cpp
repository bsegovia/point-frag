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

#include "bvh2.hpp"
#include "../bvh4/triangle4.hpp"

namespace pf
{
  template<typename T>
  int BVH2<T>::createLeaf(const Box* prims, const BuildTriangle* triangles_i, size_t nextTriangle, size_t start, size_t N)
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
      std::cout << "BVH2: loosing triangles" << std::endl;

    return int(emptyNode) | 32*int(nextTriangle-numBlocks) | int(min(numBlocks,size_t(maxLeafSize)));
  }

  template<typename T>
  void BVH2<T>::rotate(int nodeID, int maxDepth)
  {
    /*! nothing to rotate if we reached a leaf node. */
    if (nodeID < 0 || maxDepth < 0) return;
    Node& parent = node(nodeID);

    /*! rotate all children first */
    for (size_t c=0; c<2; c++)
      rotate(parent.child[c],maxDepth-1);

    /*! compute current area of all children */
    ssef sizeX = shuffle<2,3,0,1>(parent.lower_upper_x)-parent.lower_upper_x;
    ssef sizeY = shuffle<2,3,0,1>(parent.lower_upper_y)-parent.lower_upper_y;
    ssef sizeZ = shuffle<2,3,0,1>(parent.lower_upper_z)-parent.lower_upper_z;
    ssef childArea = sizeX*(sizeY + sizeZ) + sizeY*sizeZ;

    /*! transpose node bounds */
    ssef plower0,plower1,pupper0,pupper1;
    transpose(parent.lower_upper_x,parent.lower_upper_y,parent.lower_upper_z,ssef(zero),plower0,plower1,pupper0,pupper1);
    Box others[2] = { Box(plower0,pupper0), Box(plower1,pupper1) };

    /*! Find best rotation. We pick a target child of a first child,
      and swap this with an other child. We perform the best such
      swap. */
    float bestCost = pos_inf;
    int bestChild = -1, bestTarget = -1, bestOther = -1;
    for (size_t c=0; c<2; c++)
    {
      /*! ignore leaf nodes as we cannot descent into */
      if (parent.child[c] < 0) continue;
      Node& child = node(parent.child[c]);
      Box& other = others[1-c];

      /*! transpose child bounds */
      ssef clower0,clower1,cupper0,cupper1;
      transpose(child.lower_upper_x,child.lower_upper_y,child.lower_upper_z,ssef(zero),clower0,clower1,cupper0,cupper1);
      Box target0(clower0,cupper0), target1(clower1,cupper1);

      /*! compute cost for both possible swaps */
      float cost0 = halfArea(merge(other ,target1))-childArea[c];
      float cost1 = halfArea(merge(target0,other ))-childArea[c];

      if (min(cost0,cost1) < bestCost)
      {
        bestChild = (int)c;
        bestOther = (int)(1-c);
        if (cost0 < cost1) {
          bestCost = cost0;
          bestTarget = 0;
        } else {
          bestCost = cost0;
          bestTarget = 1;
        }
      }
    }

    /*! if we did not find a swap that improves the SAH then do nothing */
    if (bestCost >= 0) return;

    /*! perform the best found tree rotation */
    Node& child = node(parent.child[bestChild]);
    std::swap(parent.child  [bestOther],child.child  [bestTarget]);
    std::swap(parent.lower_upper_x[bestOther+0],child.lower_upper_x[bestTarget+0]);
    std::swap(parent.lower_upper_y[bestOther+0],child.lower_upper_y[bestTarget+0]);
    std::swap(parent.lower_upper_z[bestOther+0],child.lower_upper_z[bestTarget+0]);
    std::swap(parent.lower_upper_x[bestOther+2],child.lower_upper_x[bestTarget+2]);
    std::swap(parent.lower_upper_y[bestOther+2],child.lower_upper_y[bestTarget+2]);
    std::swap(parent.lower_upper_z[bestOther+2],child.lower_upper_z[bestTarget+2]);
    parent.lower_upper_x[bestChild+0] = min(child.lower_upper_x[0],child.lower_upper_x[1]);
    parent.lower_upper_y[bestChild+0] = min(child.lower_upper_y[0],child.lower_upper_y[1]);
    parent.lower_upper_z[bestChild+0] = min(child.lower_upper_z[0],child.lower_upper_z[1]);
    parent.lower_upper_x[bestChild+2] = max(child.lower_upper_x[2],child.lower_upper_x[3]);
    parent.lower_upper_y[bestChild+2] = max(child.lower_upper_y[2],child.lower_upper_y[3]);
    parent.lower_upper_z[bestChild+2] = max(child.lower_upper_z[2],child.lower_upper_z[3]);
  }

  template<typename T>
  void BVH2<T>::computeStatistics()
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
  float BVH2<T>::computeStatistics(int nodeID, float ap)
  {
    if (nodeID >= 0)
    {
      numNodes++;
      const Node& n = node(nodeID);
      ssef dx = shuffle<2,3,0,1>(n.lower_upper_x)-n.lower_upper_x;
      ssef dy = shuffle<2,3,0,1>(n.lower_upper_y)-n.lower_upper_y;
      ssef dz = shuffle<2,3,0,1>(n.lower_upper_z)-n.lower_upper_z;
      ssef ac = 2.0f*(dx*(dy+dz)+dy*dz);
      float sah0 = computeStatistics(n.child[0],ac[0]);
      float sah1 = computeStatistics(n.child[1],ac[1]);
      return ap*travCost + sah0 + sah1;
    }
    else {
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
  template class BVH2<Triangle4>;
}

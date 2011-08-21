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

#ifndef __PF_BVH4_H__
#define __PF_BVH4_H__

#include "../rtcore.hpp"

namespace pf
{
  /*! Multi BVH with 4 children. Each node stores the bounding box of
   * it's 4 children as well as a 32 bit child offset. If the topmost
   * bit of this offset is 0, the child is an internal node and the
   * remaining bits multiplied by offsetFactor determine its absolute
   * byte offset of the child in the node array. If the topmost bit is
   * 1, the child is a leaf node. The lower 5 bits then determine the
   * number of primitives in the leaf, and the remaining bits the ID
   * of the first primitive in the primitive array. */

  template<typename T>
    class BVH4 : public RefCount
  {
    /*! Builders and traversers need direct access to the BVH structure. */
    friend class BVH4Builder;
    friend class BVH4BuilderSpatial;
    friend class BVH2ToBVH4;
    friend class BVH4Traverser;

  public:

    /*! Triangle stored in the BVH. */
    typedef T Triangle;

    /*! Configuration of the BVH. */
    enum {
      maxDepth     = 24,       //!< Maximal depth of the BVH.
      maxLeafSize  = 31,       //!< Maximal possible size of a leaf.
      travCost     =  1,       //!< Cost of one traversal step.
      intCost      =  1,       //!< Cost of one primitive intersection.
      offsetFactor =  8,       //!< Factor to compute byte offset from offsets stored in nodes.
      emptyNode = 0x80000000   //!< ID of an empty node.
    };

    /*! BVH4 Node */
    struct Node
    {
      ssef lower_x;           //!< X dimension of upper bounds of all 4 children.
      ssef upper_x;           //!< X dimension of lower bounds of all 4 children.
      ssef lower_y;           //!< Y dimension of upper bounds of all 4 children.
      ssef upper_y;           //!< Y dimension of lower bounds of all 4 children.
      ssef lower_z;           //!< Z dimension of upper bounds of all 4 children.
      ssef upper_z;           //!< Z dimension of lower bounds of all 4 children.
      int32 child[4];         //!< Offset to the 4 children.
      //int32 padding[4];     //!< Padding to 2 cache lines does not improve performance.

      /*! Clears the node. */
      INLINE Node& clear()  {
        lower_x = pos_inf; lower_y = pos_inf; lower_z = pos_inf;
        upper_x = neg_inf; upper_y = neg_inf; upper_z = neg_inf;
        *(ssei*)child = int(emptyNode);
        return *this;
      }

      /*! Sets bounding box and ID of child. */
      INLINE void set(size_t i, const Box& bounds, int32 childID) {
        lower_x[i] = bounds.lower[0]; lower_y[i] = bounds.lower[1]; lower_z[i] = bounds.lower[2];
        upper_x[i] = bounds.upper[0]; upper_y[i] = bounds.upper[1]; upper_z[i] = bounds.upper[2];
        child[i] = childID;
      }
    };

  public:

    /*! BVH4 default constructor. */
    BVH4 () : root(int(emptyNode)), modified(true), bvhSAH(0.0f), numNodes(0), numPrims(0) {}

    /*! BVH4 destructor. */
    ~BVH4 () {
      if (nodes    ) alignedFree(nodes    ); nodes     = NULL;
      if (triangles) alignedFree(triangles); triangles = NULL;
    }

    /*! Compute the SAH cost of the BVH. */
    float getSAH()       { computeStatistics(); return bvhSAH; }

    /*! Compute number of nodes of the BVH. */
    size_t getNumNodes() { computeStatistics(); return numNodes; }

    /*! Compute number of leaves of the BVH. */
    size_t getNumLeaves() { computeStatistics(); return numLeaves; }

    /*! Compute number of primitive blocks of the BVH. */
    size_t getNumPrimBlocks() { computeStatistics(); return numPrimBlocks; }

    /*! Compute number of primitives of the BVH. */
    size_t getNumPrims() { computeStatistics(); return numPrims; }

  private:

    /*! Create a leaf node. */
    int createLeaf(const Box* prims, const BuildTriangle* triangles_i, size_t nextTriangle, size_t start, size_t N);

    /*! Rotates tree to improve SAH cost. */
    void rotate(int nodeID, int maxDepth);

    /*! Compute statistics of the BVH. */
    void computeStatistics();

    /*! Computes statistics of the BVH. */
    float computeStatistics(int nodeID, float area);

    /*! Accesses a node from the node offset. */
    INLINE       Node& node(                   size_t ofs)       { return *(Node*)((char*)nodes+offsetFactor*ofs); }

    /*! Accesses a node from the node offset. */
    INLINE const Node& node(                   size_t ofs) const { return *(Node*)((char*)nodes+offsetFactor*ofs); }

    /*! Accesses a node from the node offset. */
    INLINE const Node& node(const Node* nodes, size_t ofs) const { return *(Node*)((char*)nodes+offsetFactor*ofs); }

    /*! Transforms the ID of a node into a node offset. */
    static INLINE int id2offset(int id) {
      uint64 ofs = uint64(id)*uint64(sizeof(Node)/offsetFactor);
      if (ofs >= (1ULL<<31)) throw std::runtime_error("nodeID too large");
      return (int)ofs;
    }

    /*! Data of the BVH */
  private:
    int root;                          //!< Root node ID (can also be a leaf).
    Node* nodes;                       //!< Pointer to array of nodes.
    Triangle* triangles;               //!< Pointer to array of triangles.

    /*! Statistics about the BVH */
  private:
    bool modified;                     //!< True if statistics are invalid.
    float bvhSAH;                      //!< SAH cost of the BVH.
    size_t numNodes;                   //!< Number of internal nodes.
    size_t numLeaves;                  //!< Number of leaf nodes.
    size_t numPrimBlocks;              //!< Number of primitive blocks.
    size_t numPrims;                   //!< Number of primitives.
  };
}

#endif

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

#ifndef __PF_BVH2_H__
#define __PF_BVH2_H__

#include "../rtcore.hpp"

namespace pf
{
  /*! Binary BVH datastructure. Each node stores the bounding box of
   * it's 2 children as well as a 32 bit child offset. If the topmost
   * bit of this offset is 0, the child is an internal node and the
   * remaining bits multiplied by offsetFactor determine its absolute
   * byte offset of the child in the node array. If the topmost bit is
   * 1, the child is a leaf node. The lower 5 bits then determine the
   * number of primitives in the leaf, and the remaining bits the ID
   * of the first primitive in the primitive array. */

  template<typename T>
    class BVH2 : public RefCount
  {
    /*! Builders and traversers need direct access to the BVH structure. */
    friend class BVH2Builder;
    friend class BVH2BuilderSpatial;
    friend class BVH2ToBVH4;
    friend class BVH2Traverser;

  public:

    /*! Triangle stored in the BVH. */
    typedef T Triangle;

    /*! Configuration of the BVH. */
    enum {
      maxDepth     = 32,       //!< Maximal depth of the BVH.
      maxLeafSize  = 31,       //!< Maximal possible size of a leaf.
      travCost     =  1,       //!< Cost of one traversal step.
      intCost      =  1,       //!< Cost of one primitive intersection.
      offsetFactor =  8,       //!< Factor to compute byte offset from offsets stored in nodes.
      emptyNode = 0x80000000   //!< ID of an empty node.
    };

    /*! BVH2 Node. The nodes store for each dimension the lower and
     *  upper bounds of the left and right child in one SSE
     *  vector. This allows for a fast swap of the left and right
     *  bounds inside traversal for rays going from "right to left" in
     *  some dimension. */

    struct Node
    {
      ssef lower_upper_x;     //!< left_lower_x, right_lower_x, left_upper_x, right_upper_x
      ssef lower_upper_y;     //!< left_lower_y, right_lower_y, left_upper_y, right_upper_y
      ssef lower_upper_z;     //!< left_lower_z, right_lower_z, left_upper_z, right_upper_z
      int32 child[2];         //!< Offset to both children.
      int32 dummy[2];         //!< Padding to one cacheline (64 bytes)

      /*! Clears the node. */
      INLINE Node& clear()  {
        ssef empty = ssef(pos_inf,pos_inf,neg_inf,neg_inf);
        lower_upper_x = empty;
        lower_upper_y = empty;
        lower_upper_z = empty;
        *(ssei*)child = (int)emptyNode;
        return *this;
      }

      /*! Sets bounding box and ID of child. */
      INLINE void set(size_t i, const Box& bounds, int32 childID) {
        lower_upper_x[i+0] = bounds.lower[0];
        lower_upper_y[i+0] = bounds.lower[1];
        lower_upper_z[i+0] = bounds.lower[2];
        lower_upper_x[i+2] = bounds.upper[0];
        lower_upper_y[i+2] = bounds.upper[1];
        lower_upper_z[i+2] = bounds.upper[2];
        child[i] = childID;
      }

      /*! Returns bounds of specified child. */
      INLINE Box bounds(size_t i) {
        return Box(ssef(lower_upper_x[i+0],lower_upper_y[i+0],lower_upper_z[i+0],0.0f),
                   ssef(lower_upper_x[i+2],lower_upper_y[i+2],lower_upper_z[i+2],0.0f));
      }
    };

  public:

    /*! BVH2 default constructor. */
    BVH2 () : root(int(emptyNode)),
      nodes(NULL), allocatedNodes(0), triangles(NULL), allocatedTriangles(0),
      modified(true), bvhSAH(0.0f), numNodes(0), numLeaves(0), numPrimBlocks(0), numPrims(0) {}

    /*! BVH2 destructor. */
    ~BVH2 () {
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
      FATAL_IF (ofs >= (1ULL<<31), "nodeID too large");
      return (int)ofs;
    }

    /*! Data of the BVH */
  private:
    int root;                          //!< Root node ID (can also be a leaf).
    Node* nodes;                       //!< Pointer to array of nodes.
    size_t allocatedNodes;             //!< Number of allocated nodes.
    Triangle* triangles;               //!< Pointer to array of triangles.
    size_t allocatedTriangles;         //!< Number of allocated triangles.

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

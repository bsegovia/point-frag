// ======================================================================== //
// Copyright (C) 2011 Benjamin Segovia                                      //
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

#ifndef __PF_BVH2_HPP__
#define __PF_BVH2_HPP__

#include "bvh2_node.hpp"
#include "sys/ref.hpp"
#include "sys/platform.hpp"

namespace pf
{
  /*! Binary BVH tree */
  template <typename T>
  struct BVH2 : public RefCount
  {
    /*! Empty tree */
    BVH2(void);
    /*! Release everything */
    virtual ~BVH2(void);
    BVH2Node *node; //!< All nodes. node[0] is the root
    T *prim;        //!< Primitives the BVH sorts
    uint32 *primID; //!< Indices of primitives per leaf
    uint32 nodeNum; //!< Number of nodes in the tree
    uint32 primNum; //!< The number of primitives
  };

  template <typename T>
  BVH2<T>::BVH2(void) : node(NULL), prim(NULL), primID(NULL), nodeNum(0), primNum(0) {}

  template <typename T>
  BVH2<T>::~BVH2(void) {
    PF_ALIGNED_FREE(this->node);
    PF_SAFE_DELETE_ARRAY(this->primID);
    PF_SAFE_DELETE_ARRAY(this->prim);
  }

  /*! Options to compile the BVH2 */
  struct BVH2BuildOption {
    INLINE BVH2BuildOption(void) {}
    INLINE BVH2BuildOption(uint32 minPrimNum,
                           uint32 maxPrimNum,
                           uint32 SAHIntersectionCost,
                           uint32 SAHTraversalCost) :
      minPrimNum(minPrimNum),
      maxPrimNum(maxPrimNum),
      SAHIntersectionCost(SAHIntersectionCost),
      SAHTraversalCost(SAHTraversalCost) {}
    uint32 minPrimNum;          //!< Minimum number of primitives per leaf
    uint32 maxPrimNum;          //!< Maximum number of primitives per leaf
    float SAHIntersectionCost;  //!< Estimated cost to traverse the leaf
    float SAHTraversalCost;     //!< Estimated cost to intersect a primitive
  };

  /*! Default options (mostly suitable for ray tracing) */
  extern const BVH2BuildOption defaultBVH2Options;

  /*! Compile a BVH */
  template <typename T>
  void buildBVH2(const T *t, uint32 primNum, BVH2<T> &bvh,
                 const BVH2BuildOption &option = defaultBVH2Options);

} /* namespace pf */

#endif /* __PF_BVH2_HPP__ */


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

#ifndef __PF_BVH_HPP__
#define __PF_BVH_HPP__

#include "sys/platform.hpp"
#include "sys/ref.hpp"
#include "math/vec.hpp"

namespace pf
{
  /*! 32 bytes BVH Node */
  struct ALIGNED(16) BVH2Node
  {
    /*! Lower bound */
    INLINE const vec3f& getMin(void)  const { return this->pmin; }
    /*! Upper bound */
    INLINE const vec3f& getMax(void)  const { return this->pmax; }
    /*! Bounding box pointer */
    INLINE const float* getAABB(void) const { return &this->pmin[0]; }
    /*! Child offset */
    INLINE uint32 getOffset(void) const { return this->offsetFlag & ~BIT_FLAG; }
    /*! Number of primitives if this is a leaf */
    INLINE uint32 getPrimNum(void) const { return this->primNum & ~BIT_FLAG; }
    /*! Get the primitive ID index */
    INLINE uint32 getPrimID(void)  const { return this->primID; }
    /*! Get the main axis if this is not a leaf */
    INLINE uint32 getAxis(void)   const { return this->axis; }
    /*! Indicate if this is a leaf */
    INLINE bool isLeaf(void)     const  { return this->offsetFlag & BIT_FLAG; }
    /*! Get the dimension of the node */
    INLINE vec3f getExtent(void) const  { return this->pmax - this->pmin; }
    /*! Set the child offset */
    INLINE void setOffset(uint32 _offset) {
      this->offsetFlag &= BIT_FLAG;
      this->offsetFlag |= _offset;
    }
    /*! Set the number of primitives */
    INLINE void setPrimNum(uint32 _primNum) {
      this->primNum &= BIT_FLAG;
      this->primNum |= _primNum;
    }
    /*! Set the primitive ID index */
    INLINE void setPrimID(uint32 _primID) { this->primID = _primID; }
    /*! Set the main sort axis for a non-leaf */
    INLINE void setAxis(uint32 _axis)   { this->axis = _axis; }
    /*! Set lower bound */
    INLINE void setMin(const vec3f &_pmin) { this->pmin = _pmin; }
    /*! Set upper bound */
    INLINE void setMax(const vec3f &_pmax) { this->pmax = _pmax; }
    /*! Set the node as a leaf */
    INLINE void setAsLeaf(void)     { this->offsetFlag |= BIT_FLAG; }
    /*! Set the node as a non-leaf */
    INLINE void setAsNonLeaf(void)  { this->offsetFlag &= ~BIT_FLAG; }
    /*! Lower bound */
    vec3f pmin;
    /*! Number of primitives (for leaves) or child offset (for the rest) */
    union {
      uint32 offsetFlag;
      uint32 primNum;
    };
    /*! Upper bound */
    vec3f pmax;
    /*! Primitive ID or sorting axis */
    union {
      uint32 primID;
      uint32 axis;
    };
    static const uint32 BIT_FLAG = 0x80000000;
  };

  /*! Binary BVH tree */
  template <typename T>
  struct BVH2 : public RefCount
  {
    BVH2(void);     //!< Empty tree
    ~BVH2(void);    //!< Release everything
    BVH2Node *node; //!< All nodes. node[0] is the root
    T *prim;        //!< Primitives the BVH sorts
    uint32 *primID; //!< Indices of primitives per leaf
    uint32 nodeNum; //!< Number of nodes in the tree
    uint32 primNum; //!< The number of primitives
  };

  /*! Compile a BVH */
  template <typename T> bool BVH2Build(const T *t, uint32 primNum, BVH2<T> &bvh);

} /* namespace pf */

#endif /* __PF_BVH_HPP__ */


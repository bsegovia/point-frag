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

#ifndef __PF_BVH2_TRAVERSER_HPP__
#define __PF_BVH2_TRAVERSER_HPP__

#include "intersector.hpp"

namespace pf
{
  // Structure to traverse
  template <typename T> struct BVH2;

  /*! Represents any kind of intersectable geometry that we are going to
   *  traverse with rays or packet of rays
   */
  template <typename T>
  class BVH2Traverser : public Intersector
  {
  public:
    /*! We keep a reference on the BVH */
    BVH2Traverser(Ref< BVH2<T> > bvh) : bvh(bvh) {}

    /*! Traverse routine for rays */
    virtual void traverse(const Ray &ray, Hit &hit) const;

    /*! Traverse routine for ray packets. Return u,v,t and ID of primitive of
     *  for each ray of the packet
     */
    virtual void traverse(const RayPacket &pckt, PacketHit &hit) const;

    /*! Shadow ray routine for rays. True if occluded */
    virtual bool occluded(const Ray &ray) const;

    /*! The BVH we intersect */
    Ref< BVH2<T> > bvh;
  };

} /* namespace pf */

#endif /* __PF_BVH2_TRAVERSER_HPP__ */


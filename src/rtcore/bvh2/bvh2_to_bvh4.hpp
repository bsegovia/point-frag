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

#ifndef __PF_BVH2_TO_BVH4_H__
#define __PF_BVH2_TO_BVH4_H__

#include "bvh2.hpp"
#include "../bvh4/bvh4.hpp"
#include "../bvh4/triangle4.hpp"
#include "../common/builder.hpp"

namespace pf
{
  /* Converts a BVH2 into a BVH4. */
  class BVH2ToBVH4 : public Builder
  {
  public:

    /*! API entry function for the converter */
    static Ref<BVH4<Triangle4> > convert(Ref<BVH2<Triangle4> >& bvh2);

  public:

    /*! Construction. */
    BVH2ToBVH4(Ref<BVH2<Triangle4> >& bvh2, Ref<BVH4<Triangle4> >& bvh4);

    /*! recursively converts BVH2 into BVH4 */
    int recurse(int parent, int depth);

  public:
    Ref<BVH2<Triangle4> > bvh2;   //!< source BVH2
    Ref<BVH4<Triangle4> > bvh4;   //!< target BVH4
  };
}

#endif

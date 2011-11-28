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

#include "math/bbox.hpp"

namespace pf
{
  // Simplistic. We just create segments from OBJ only right now
  struct Obj;

  /*! This encapsulates one draw call. Right now, it is super simple since we
   *  only display RendererObj but it will become better in the future. The idea
   *  is simply to represent one GL draw. In this simplistic version, this is no
   *  more that a sequence of indices from an index buffer
   */
  struct RendererSegment
  {
    BBox3f bbox;        //!< Bounding box of the triangles
    uint32 first, last; //!< First and last index in the index buffer
    uint32 matID;       //!< Material ID
  };

} /* namespace pf */


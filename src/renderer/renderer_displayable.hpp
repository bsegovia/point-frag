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

#ifndef __PF_RENDERER_DISPLAYABLE_HPP__
#define __PF_RENDERER_DISPLAYABLE_HPP__

#include "renderer_object.hpp"

namespace pf
{
  /*! We will just cast the object based on this enum to properly dispatch the
   *  code
   */
  enum RendererDisplayableType {
    RN_DISPLAYABLE_WAVEFRONT = 0,
    RN_DISPLAYABLE_SET       = 1
  };

  /*! Common interface for all objects that can be drawn on screen. We do not
   *  really use any pure interface for that since the dispatch will be quite
   *  hardcoded. However, it is convenient to have a common interface
   */
  class RendererDisplayable : public RendererObject
  {
  public:
    /*! We store the object type to statically dispatch the code */
    RendererDisplayable(Renderer &renderer, RendererDisplayableType type) :
      RendererObject(renderer), type(type) {}
    /*! Return the object type */
    INLINE RendererDisplayableType getType(void) const { return this->type; }
  private:
    const RendererDisplayableType type;
  };

} /* namespace pf */

#endif /* __PF_RENDERER_DISPLAYABLE_HPP__ */

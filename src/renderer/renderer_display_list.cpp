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

#include "renderer_display_list.hpp"
#include "renderer_obj.hpp"

namespace pf
{
  RendererDisplayList::RendererDisplayList(Renderer &renderer) :
    RendererObject(renderer) {}
  RendererDisplayList::~RendererDisplayList(void) {}
  void RendererDisplayList::onCompile(void) {}
  void RendererDisplayList::onUnreferenced(void) {}
  void RendererDisplayList::add(RendererObj *obj, const float *m) {
    PF_ASSERT(this->isCompiled() == false);
    Elem elem;
    if (m) {
      elem.isIdentity = 0;
      elem.model = mat4x4f(m[0], m[1], m[2], m[3],
                           m[4], m[5], m[6], m[7],
                           m[8], m[9], m[10],m[11],
                           m[12],m[13],m[14],m[15]);
    } else {
      elem.isIdentity = 1;
      elem.model = mat4x4f(one);
    }
    elem.object = obj;
    this->objects.push_back(elem);
  }

} /* namespace pf */


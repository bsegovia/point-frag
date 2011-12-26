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

#ifndef __PF_RENDERER_DISPLAY_LIST_HPP__
#define __PF_RENDERER_DISPLAY_LIST_HPP__

#include "renderer_displayable.hpp"
#include "math/matrix.hpp"
#include "sys/ref.hpp"
#include "sys/list.hpp"

namespace pf
{
  class RendererObj; // Wavefront mesh
  class RendererSet; // Set of objects to render

  /*! Stores the objects to display */
  class RendererDisplayList : public RendererObject
  {
  public:
    /*! Object to display */
    struct Elem {
      Ref<RendererDisplayable> object;//!< Object to render
      mat4x4f model;                  //!< Model matrix
      uint32 isIdentity;              //!< True of the user provides NULL
    };
    /*! Create an emtpy display list */
    RendererDisplayList(Renderer &renderer);
    /*! Destroy the list */
    ~RendererDisplayList(void);
    /*! Append an wavefront object with the given model 4x4 matrix */
    void add(RendererObj *obj, const float *model = NULL);
    /*! Append a set with the given model 4x4 matrix */
    void add(RendererSet *set, const float *model = NULL);
    /*! Get the list of objects */
    INLINE list<Elem> &getList(void) { return this->objects; }
  private:
    /*! Implements base class method */
    void onCompile(void);
    /*! Implements base class method */
    void onUnreferenced(void);
    /*! All the objects to render */
    list<Elem> objects;
    PF_CLASS(RendererDisplayList);
  };
} /* namespace pf */

#endif /* __PF_RENDERER_DISPLAY_LIST_HPP__ */

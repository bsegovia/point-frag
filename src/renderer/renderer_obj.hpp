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

#ifndef __RENDERER_OBJ_HPP__
#define __RENDERER_OBJ_HPP__

#include "renderer/texture.hpp"
#include "math/bbox.hpp"
#include "GL/gl3.h"

namespace pf
{
  /*! Renderer owns all RendererObj */
  class Renderer;

  /*! Entity used for rendering of OBJ models */
  class RendererObj : public RefCount {
  public:
    friend class Renderer;
    /*! Index of the first and last triangle index */
    struct Group { GLuint first, last; };
    Renderer &renderer;   //!< A RendererObj belongs to a renderer
    Ref<Texture2D> *tex;  //!< One texture per group of triangles
    std::string *texName; //!< All texture names
    BBox3f *bbox;         //!< One bounding box for each group
    Group *grp;           //!< Indices of each group
    GLuint vertexArray;   //!< Vertex declaration
    GLuint arrayBuffer;   //!< Vertex data (positions, normals...)
    GLuint elementBuffer; //!< Indices
    GLuint grpNum;        //!< Number of groups in the model
    GLuint topology;      //!< Mostly triangle or triangle strip
    Ref<Task> texLoading; //!< XXX To load the textures
    MutexSys mutex;       //!< XXX just to play with async load
    /*! Valid means it is in GL */
    INLINE bool isValid(void) { return this->grpNum > 0; }
    /*! Display it using the renderer */
    void display(void);
    /*! Note that this object actually belongs to a renderer */
    RendererObj(Renderer &renderer, const FileName &fileName);
    /*! Release it (still from a renderer */
    ~RendererObj(void);
  };
}

#endif /* __RENDERER_OBJ_HPP__ */


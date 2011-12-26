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

#ifndef __PF_RENDERER_OBJ_HPP__
#define __PF_RENDERER_OBJ_HPP__

#include "renderer/texture.hpp"
#include "renderer/renderer_displayable.hpp"
#include "rt/intersector.hpp"
#include "math/vec.hpp"
#include "sys/array.hpp"
#include "GL/gl3.h"

namespace pf
{
  struct Obj;                // We build the renderer obj from a Wavefront OBJ
  struct RendererSegment;    // To make a partition of the objects to display
  struct RendererObjOGLData; // Data to upload to OGL

  /*! Entity used for rendering of OBJ models */
  class RendererObj : public RendererDisplayable
  {
  public:
    /*! Renderer mapping of a OBJ material */
    struct Material
    {
      std::string name;
      std::string name_Ka;
      std::string name_Kd;
      std::string name_D;
      std::string name_Bump;
      Ref<Texture2D> map_Ka;
      Ref<Texture2D> map_Kd;
      Ref<Texture2D> map_D;
      Ref<Texture2D> map_Bump;
      vec3f amb;
      vec3f diff;
      vec3f spec;
      float km;
      float reflect;
      float refract;
      float trans;
      float shiny;
      float glossy;
      float refract_index;
    };

    /*! Note that this object actually belongs to a renderer */
    RendererObj(Renderer &renderer, const Obj &obj);
    /*! Release it (still from a renderer */
    ~RendererObj(void);
    /*! Valid means there is something to display */
    INLINE bool isValid(void) { return this->segments.size() > 0; }
    /*! We provide the indices of the visible segments */
    void display(const array<uint32> &visible, uint32 visibleNum);
    array<Material> mat;             //!< All the material of the object
    array<RendererSegment> segments; //!< All the sub-meshes to display
    Ref<Intersector> intersector; //!< Optional BVH
    Ref<Task> texLoading;         //!< Load the textures
    Ref<RefCount> sharedData;     //!< May be needed by other renderer objects
    GLuint vertexArray;           //!< Vertex declaration
    GLuint arrayBuffer;           //!< Vertex data (positions, normals...)
    GLuint elementBuffer;         //!< Indices
    GLuint topology;              //!< Mostly triangle or triangle strip
    uint32 properties;            //!< occluder == has a BVH
    MutexSys mutex;               //!< XXX just to play with async load
  private:
    virtual void onCompile(void);
    virtual void onUnreferenced(void);
    friend class Renderer;
  };
}

#endif /* __PF_RENDERER_OBJ_HPP__ */


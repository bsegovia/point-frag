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

#ifndef __RENDERER_DRIVER_HPP__
#define __RENDERER_DRIVER_HPP__

#include "renderer/ogl.hpp"
#include "renderer/texture.hpp"
#include "renderer/renderer_obj.hpp"
#include "math/matrix.hpp"
#include "math/bbox.hpp"
#include <unordered_map>
#include <string>
#include <cassert>

namespace pf
{
  /*! Will basically render everything */
  class RendererDriver : public OGL
  {
  public:
    /*! Constructor */
    RendererDriver(void);
    /*! Destructor */
    virtual ~RendererDriver(void);
    /*! Call it when window size changes */
    void resize(int w, int h);

    /*! Display bounding boxes in wireframe */
    void displayBBox(const BBox3f *bbox, int n = 1, const vec4f *c = NULL);
    /*! Uniform values used in the renderer */
    INLINE void setDefaultDiffuseCol(vec4f c)  {this->defaultDiffuseCol = c;}
    INLINE void setDefaultSpecularCol(vec4f c) {this->defaultSpecularCol = c;}
    INLINE void setMVP(const mat4x4f &m) {this->MVP = m;}

    /*! For vertex arrays */
    static const GLuint ATTR_POSITION = 0;
    static const GLuint ATTR_TEXCOORD = 1;
    static const GLuint ATTR_COLOR    = 2;
    static const GLuint ATTR_NORMAL   = 3;
    static const GLuint ATTR_TANGENT  = 4;

    /*! FBO (and related buffers) for deferred shading */
    struct {
      GLuint fbo;         //!< FBO with all buffers binded
      GLuint diffuse_exp; //!< diffuse color + spec exponent
      GLuint radiance;    //!< radiance value
      GLuint pxpypz_nx;   //!< xyz position + normal.x
      GLuint vxvy_nynz;   //!< velocity + normal.y + normal.z
      GLuint zbuffer;     //!< depth stencil
    } gbuffer;
    void initGBuffer(int w, int h);
    void destroyGBuffer(void);

    /*! To display a quad */
    struct {
      GLuint program;
      GLuint uDiffuse, uMVP;
      GLuint vertexArray, arrayBuffer;
    } quad;
    void initQuad(void);
    void destroyQuad(void);

    /*! To display object with one color */
    struct {
      GLuint program;
      GLuint uCol, uMVP;
      GLuint vertexArray, arrayBuffer;
    } plain;
    void initPlain(void);
    void destroyPlain(void);

    /*! GLSL program to display object with diffuse texture only */
    struct {
      GLuint program;
      GLuint uDiffuse, uMVP;
    } diffuse;
    void initDiffuse(void);
    void destroyDiffuse(void);

  private:
    /*! Store the texture per name (only its base name is taken into account) */
    //std::unordered_map<std::string, Ref<Texture2D>> texMap;
    /*! Default colors */
    vec4f defaultDiffuseCol, defaultSpecularCol;
    /*! Model view projection */
    mat4x4f MVP;
    /*! Helper to create buffers for deferred shading */
    GLuint makeGBufferTexture(int internal, int w, int h, int data, int type);
    /*! Helper to build a complete program from file */
    GLuint buildProgram(const std::string &prefix, bool useVertex, bool useGeometry, bool useFragment);
    /*! Helper to build a complete program from memory */
    GLuint buildProgram(const char *vertSource, const char *geomSource, const char *fragSource);
  };

  /*! Where you may find data files and shaders */
  extern const char *defaultPath[];
  extern const size_t defaultPathNum;
}
#endif /* __RENDERER_DRIVER_HPP__ */


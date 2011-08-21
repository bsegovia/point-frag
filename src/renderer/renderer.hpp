#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "ogl.hpp"
#include "math/bbox.hpp"
#include "math/matrix.hpp"
#include <unordered_map>
#include <string>

namespace pf
{
  /*! Will basically offer everything we need to render */
  struct Renderer : OGL {
    Renderer(void);
    virtual ~Renderer(void);
    /*! Call it when window size changes */
    void resize(int w, int h);
    /*! Display bounding boxes in wireframe */
    void displayBBox(const BBox3f *bbox, int n, const vec4f *c = NULL);
    INLINE void setDefaultDiffuseCol(vec4f c)  {this->defaultDiffuseCol = c;}
    INLINE void setDefaultSpecularCol(vec4f c) {this->defaultSpecularCol = c;}
    INLINE void setMVP(const mat4x4f &m) {this->MVP = m;}

    /*! For vertex arrays */
    static const GLuint ATTR_POSITION = 0;
    static const GLuint ATTR_TEXCOORD = 1;
    static const GLuint ATTR_COLOR = 2;
    static const GLuint ATTR_NORMAL = 3;
    static const GLuint ATTR_TANGENT = 4;

    /*! FBO (and related buffers) for deferred shading */
    struct {
      GLuint fbo;          /*! FBO with all buffers binded*/
      GLuint diffuse_exp;  /*! diffuse color + spec exponent */
      GLuint radiance;     /*! radiance value*/
      GLuint pxpypz_nx;    /*! xyz position + normal.x */
      GLuint vxvy_nynz;    /*! velocity + normal.y + normal.z */
      GLuint zbuffer;      /*! depth stencil */
    } gbuffer;
    void initGBuffer(int w, int h);
    void destroyGBuffer(void);

    /*! Fullscreen quad */
    struct {
      GLuint vertexArray;
      GLuint arrayBuffer;
    } quadBuffer;
    /*! GLSL program to display a quad */
    struct {
      GLuint program;
      GLuint uDiffuse, uMVP;
    } quad;
    /*! GLSL program to display object with one color */
    struct {
      GLuint program;
      GLuint uCol, uMVP;
    } plain;
    void initPlain(void);
    void destroyPlain(void);
    /*! GLSL program to display object with diffuse texture only */
    struct {
      GLuint program;
      GLuint uDiffuse, uMVP;
    } diffuse;

  private:
    /*! Store the texture ID per name */
    std::unordered_map<std::string, GLuint> texMap;
    /*! Default colors */
    vec4f defaultDiffuseCol, defaultSpecularCol;
    /*! Model view projection */
    mat4x4f MVP;

    ///////////////////////////////////////////////////////////////////////
    // Some helper functions
    ///////////////////////////////////////////////////////////////////////
    /*! Helper to create buffers for deferred shading */
    GLuint makeGBufferTexture(int internal, int w, int h, int data, int type);
    /*! Build a complete program from file */
    GLuint buildProgram(const std::string &prefix, bool useVertex, bool useGeometry, bool useFragment);
    /*! Build a complete program from the source */
    GLuint buildProgram(const char *vertSource, const char *geomSource, const char *fragSource);
  };
}

#endif /* __RENDERER_HPP__ */


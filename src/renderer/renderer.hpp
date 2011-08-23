#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "renderer/ogl.hpp"
#include "renderer/texture.hpp"
#include "math/bbox.hpp"
#include "math/matrix.hpp"
#include <unordered_map>
#include <string>

namespace pf
{
  /*! Entity used for rendering of OBJ models */
  struct RenderObj : RefCount {
    GLuint vertexArray;   /*! Vertex declaration */
    GLuint vertexBuffer;  /*! Vertex data (positions, normals...) */
    GLuint elementBuffer; /*! Indices */
    GLuint grpNum;        /*! Number of groups in the model */
    Ref<Texture2D> *tex;  /*! One texture per group of triangles */
    BBox3f *bbox;         /*! One bounding box for each group */
    struct { GLuint first, last; } *grp; /*! Indices of each group */
  };

  /*! Will basically render everything */
  struct Renderer : OGL
  {
    /*! Constructor */
    Renderer(void);
    /*! Destructor */
    virtual ~Renderer(void);
    /*! Call it when window size changes */
    void resize(int w, int h);
    /*! Create a model from an Obj file */
    Ref<RenderObj> createRenderObj(const FileName &fileName);

    /*! Display bounding boxes in wireframe */
    void displayBBox(const BBox3f *bbox, int n = 1, const vec4f *c = NULL);
    /*! Display a model */
    void displayRenderObj(const RenderObj &model);

    /*! Uniform values used in the renderer */
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
    /*! Store the texture per name (only its base name is taken into account) */
    std::unordered_map<Ref<Texture2D>, GLuint> texMap;
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
}

#endif /* __RENDERER_HPP__ */


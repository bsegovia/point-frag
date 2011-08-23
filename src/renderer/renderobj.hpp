#ifndef __RENDEROBJ_HPP__
#define __RENDEROBJ_HPP__

#include "renderer/texture.hpp"
#include "math/bbox.hpp"
#include "GL/gl3.h"

namespace pf
{
  /*! Renderer owns all RenderObj */
  class Renderer;

  /*! Entity used for rendering of OBJ models */
  class RenderObj : RefCount {
  public:
    friend class Renderer;
    GLuint vertexArray;   /*! Vertex declaration */
    GLuint vertexBuffer;  /*! Vertex data (positions, normals...) */
    GLuint elementBuffer; /*! Indices */
    GLuint grpNum;        /*! Number of groups in the model */
    GLuint topology;      /*! Mostly triangle or triangle strip */
    Ref<Texture2D> *tex;  /*! One texture per group of triangles */
    BBox3f *bbox;         /*! One bounding box for each group */
    struct { GLuint first, last; } *grp; /*! Indices of each group */
    /*! May be invalid if we do not find the file */
    INLINE bool isValid(void) { return this->grpNum > 0; }
  private:
    /*! Note that this object actually belongs to a renderer */
    RenderObj(Renderer &renderer, const FileName &fileName);
    /*! Release it (still from a renderer */
    ~RenderObj(void);
  };
}

#endif /* __RENDEROBJ_HPP__ */


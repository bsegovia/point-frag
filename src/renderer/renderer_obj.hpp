#ifndef __RENDERER_OBJ_HPP__
#define __RENDERER_OBJ_HPP__

#include "renderer/texture.hpp"
#include "math/bbox.hpp"
#include "GL/gl3.h"

namespace pf
{
  /*! RendererDriver owns all RendererObj */
  class RendererDriver;

  /*! Entity used for rendering of OBJ models */
  class RendererObj : public RefCount {
  public:
    friend class RendererDriver;
    /*! Index of the first and last triangle index */
    struct Group { GLuint first, last; };
    RendererDriver &renderer;   //!< A RendererObj belongs to a renderer
    GLuint vertexArray;   //!< Vertex declaration
    GLuint arrayBuffer;   //!< Vertex data (positions, normals...)
    GLuint elementBuffer; //!< Indices
    GLuint grpNum;        //!< Number of groups in the model
    GLuint topology;      //!< Mostly triangle or triangle strip
    Ref<Texture2D> *tex;  //!< One texture per group of triangles
    BBox3f *bbox;         //!< One bounding box for each group
    Group *grp;           //!< Indices of each group
    /*! Valid means it is in GL */
    INLINE bool isValid(void) { return this->grpNum > 0; }
    /*! Display it using the renderer */
    void display(void);
    /*! Note that this object actually belongs to a renderer */
    RendererObj(RendererDriver &renderer, const FileName &fileName);
    /*! Release it (still from a renderer */
    ~RendererObj(void);
  };
}

#endif /* __RENDERER_OBJ_HPP__ */


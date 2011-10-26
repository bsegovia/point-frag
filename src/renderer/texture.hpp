#ifndef __PF_TEXTURE_HPP__
#define __PF_TEXTURE_HPP__

#include "sys/filename.hpp"
#include "sys/ref.hpp"
#include "renderer/GL/gl3.h"

namespace pf
{
  /*! Its owns all the textures */
  class RendererDriver;

  /*! Small wrapper around a GL 2D texture */
  struct Texture2D : RefCount
  {
    /*! Create texture from an image. mipmap == true will create the mipmaps */
    Texture2D(RendererDriver &renderer, const FileName &path, bool mipmap = true);
    /*! Release it from OGL */
    ~Texture2D(void);
    /*! Valid means it is actually in GL */
    INLINE bool isValid(void) const {return this->handle != 0;}
    RendererDriver &renderer;/*! Owner of this class */
    GLuint handle;     /*! Texture object itself */
    GLuint fmt;        /*! GL format of the texture */
    GLuint w, h;       /*! Dimensions of level 0 */
    GLuint minLevel;   /*! Minimum level we loaded */
    GLuint maxLevel;   /*! Maximum level we loaded */
  };
}

#endif /* __PF_TEXTURE_HPP__ */


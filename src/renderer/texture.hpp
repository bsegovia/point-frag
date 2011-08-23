#ifndef __PF_TEXTURE_HPP__
#define __PF_TEXTURE_HPP__

#include "sys/filename.hpp"
#include "sys/ref.hpp"
#include "renderer/GL/gl3.h"

namespace pf
{
  /*! Small wrapper around a GL 2D texture */
  struct Texture2D : RefCount
  {
    /*! Create texture from an image. mipmap == true will create the mipmaps */
    Texture2D(const FileName &path, bool mipmap = true);
    ~Texture2D(void);
    GLuint handle;   /*! Texture object itself */
    GLuint fmt;      /*! GL format of the texture */
    GLuint w, h;     /*! Dimensions of level 0 */
    GLuint minLevel; /*! Minimum level we loaded */
    GLuint maxLevel; /*! Maximum level we loaded */
  };

  GLuint loadTexture(const FileName &fileName,
                     GLuint *fmt = NULL,
                     GLuint *w = NULL,
                     GLuint *h = NULL,
                     GLuint *minLevel = NULL,
                     GLuint *maxLevel = NULL,
                     bool mipmap = true);
}

#endif /* __PF_TEXTURE_HPP__ */


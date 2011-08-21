#ifndef __PF_TEXTURE_HPP__
#define __PF_TEXTURE_HPP__

#include "renderer/GL/gl3.h"
#include "sys/filename.hpp"
#include <string>

namespace pf
{
  GLuint loadTexture(const FileName &path);
}

#endif /* __PF_TEXTURE_HPP__ */


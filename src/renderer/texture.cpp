#include "texture.hpp"
#include "renderer/ogl.hpp"
#include "math/math.hpp"
#include "image/stb_image.hpp"

#include <cstring>
#include <algorithm>

#define OGL_NAME ogl
namespace pf
{
  static unsigned char *doMipmap(const unsigned char *src,
                                 int w, int h,
                                 int mmW, int mmH,
                                 int compNum)
  {
    // Use malloc since stb_image also uses malloc
    unsigned char *dst = (unsigned char*) malloc(mmW*mmH*compNum);
    for (int y = 0; y < mmH; ++y)
    for (int x = 0; x < mmW; ++x) {
      const int offset = (x + y*mmW) * compNum;
      const int offset0 = (2*x+0 + 2*y*w+0) * compNum;
      const int offset1 = (2*x+1 + 2*y*w+w) * compNum;
      const int offset2 = (2*x+0 + 2*y*w+w) * compNum;
      const int offset3 = (2*x+1 + 2*y*w+0) * compNum;
      for (int c = 0; c < compNum; ++c) {
        const float f = float(src[offset0+c]) + 
                        float(src[offset1+c]) +
                        float(src[offset2+c]) +
                        float(src[offset3+c]);
        dst[offset+c] = (unsigned char) (f * 0.25f);
      }
    }
    return dst;
  }

  static void revertTGA(unsigned char *img, int w, int h, int compNum)
  {
    for (int y = 0; y < h / 2; ++y)
    for (int x = 0; x < w; ++x) {
      const int offset = compNum * (y*w + x);
      const int mirror = compNum * ((h-y-1)*w + x);
      for (int c = 0; c < compNum; ++c)
        std::swap(img[offset+c], img[mirror+c]);
    }
  }

  GLuint loadTexture(const FileName &fileName,
                     GLuint *format,
                     GLuint *width, GLuint *height,
                     GLuint *minLevel, GLuint *maxLevel,
                     bool mipmap)
  {
    int w, h, comp, fmt = GL_RGBA;
    unsigned char *img = stbi_load(std::string(fileName).c_str(), &w, &h, &comp, 0);
    if (img == NULL)
      return 0;

    // Revert TGA images
    if (fileName.ext() == "tga")
      revertTGA(img, w, h, comp);
    const int levelNum = (int) max(log2(w), log2(h));
    switch (comp) {
      case 3: fmt = GL_RGB; break;
      case 4: fmt = GL_RGBA; break;
      default: FATAL("unsupported number of componenents");
    };
    if (width) *width = w;
    if (height) *height = h;
    if (format) *format = fmt;
    if (minLevel) *minLevel = 0;
    if (maxLevel) *maxLevel = levelNum;

    // Load the texture
    GLuint texName = 0;
    R_CALL (GenTextures, 1, &texName);
    R_CALL (ActiveTexture, GL_TEXTURE0);
    R_CALL (BindTexture, GL_TEXTURE_2D, texName);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int lvl = 0;
    for (;;) {
      R_CALL (TexImage2D, GL_TEXTURE_2D, lvl, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, img);
      if (lvl >= levelNum || mipmap == false) {
        free(img);
        break;
      } else {
        const int mmW = max(w/2,1), mmH = max(h/2, 1);
        unsigned char *mipmap = doMipmap(img, w, h, mmW, mmH, comp);
        free(img);
        w = mmW;
        h = mmH;
        img = mipmap;
      }
      ++lvl;
    }
    R_CALL (BindTexture, GL_TEXTURE_2D, 0);
    return texName;
  }

  Texture2D::Texture2D(const FileName &path, bool mipmap) {
    this->handle = loadTexture(path, &this->fmt,
                               &this->w, &this->h,
                               &this->minLevel, &this->maxLevel,
                               mipmap);
  }
  Texture2D::~Texture2D(void) { R_CALL (DeleteTextures, 1, &this->handle); }

}
#undef OGL_NAME


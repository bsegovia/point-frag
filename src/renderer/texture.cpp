#include "renderer/texture.hpp"
#include "renderer/renderer_driver.hpp"
#include "sys/tasking.hpp"
#include "image/stb_image.hpp"
#include "math/math.hpp"

#include <cstring>
#include <algorithm>

#define OGL_NAME (&renderer)
namespace pf
{
  static unsigned char *doMipmap(const unsigned char *src,
                                 int w, int h,
                                 int mmW, int mmH,
                                 int compNum)
  {
    unsigned char *dst = (unsigned char*) pf::malloc(mmW*mmH*compNum);
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

  Texture2D::Texture2D(RendererDriver &renderer, const FileName &fileName, bool mipmap)
    : renderer(renderer)
  {
    int comp;
    unsigned char *img = stbi_load(std::string(fileName).c_str(),
                                   (int*) &this->w, (int*) &this->h, &comp, 0);
    this->fmt = this->minLevel = this->maxLevel = this->handle = 0;
    if (img != NULL) {
      // Revert TGA images
      this->fmt = GL_RGBA;
      //if (fileName.ext() == "tga") TODO check the image loader
      revertTGA(img, w, h, comp);
      const int levelNum = (int) max(log2(float(w)), log2(float(h)));
      switch (comp) {
        case 3: this->fmt = GL_RGB; break;
        case 4: this->fmt = GL_RGBA; break;
        default: FATAL("unsupported number of componenents");
      };
      this->minLevel = 0;
      this->maxLevel = levelNum;

      // Load the texture
      R_CALL (GenTextures, 1, &this->handle);
      R_CALL (ActiveTexture, GL_TEXTURE0);
      R_CALL (BindTexture, GL_TEXTURE_2D, this->handle);
      R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      int lvl = 0;
      for (;;) {
        R_CALL (TexImage2D, GL_TEXTURE_2D, lvl, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, img);
        if (lvl >= levelNum || mipmap == false) {
          pf::free(img);
          break;
        } else {
          const int mmW = max(w/2, 1u), mmH = max(h/2, 1u);
          unsigned char *mipmap = doMipmap(img, w, h, mmW, mmH, comp);
          pf::free(img);
          w = mmW;
          h = mmH;
          img = mipmap;
        }
        ++lvl;
      }
      R_CALL (BindTexture, GL_TEXTURE_2D, 0);
    } else
      this->w = this->h = 0;
  }

  Texture2D::~Texture2D(void) {
    if (this->isValid()) R_CALL (DeleteTextures, 1, &this->handle);
  }

  TextureState TextureStreamer::getTextureState(const char *name) {
    Lock<MutexSys> lock(mutex);
    return this->getTextureStateUnlocked(name);
  }

  TextureState TextureStreamer::getTextureStateUnlocked(const char *name) {
    auto it = texMap.find(name);
    if (it == texMap.end())
      return TextureState();
    else
      return it->second;
  }

  /*! Contain the texture data (format + data) that we will provide to OGL */
  class TextureLoadData
  {
  public:
    TextureLoadData(const char *name);
    ~TextureLoadData(void);
    const char **texels; //!< All mip-map level texels
    const int *w, *h;    //!< Dimensions of all mip-maps
    int levelNum;        //!< Number of mip-maps
    const char *name;    //!< Name of the file containing the data
  };

  TextureLoadData::TextureLoadData(const char *name)
  {

  }

  TextureLoadData::~TextureLoadData(void)
  {

  }

  class TaskTextureLoad;    //!< Load the textures from the disk
  class TaskTextureLoadOGL; //!< Upload the mip level to OGL

  /*! First task simply loads the texture from memory and build all the
   *  mip-maps. We store everything in the texture load data
   */
  class TaskTextureLoad : public Task
  {
  public:
    INLINE TaskTextureLoad(const char *name, TextureStreamer &streamer) :
      Task("TaskTextureLoad"), name(name), streamer(streamer) {}
    virtual Task* run(void);
    const char *name;          //!< File to load
    TextureStreamer &streamer; //!< Streamer that handles streaming
  };

  /*! Second task finishes the work by creating the OGL texture */
  class TaskTextureLoadOGL : public TaskMain
  {
  public:
    INLINE TaskTextureLoadOGL(TextureLoadData *data) :
      TaskMain("TaskTextureLoadOGL"), data(data), streamer(streamer) {}
    virtual Task* run(void);
    TextureLoadData *data;     //!< Data to upload
    const char *name;          //!< Texture to load
    TextureStreamer &streamer; //!< Streamer that handles the texture
  };

  Task *TaskTextureLoad::run(void) {
    return NULL;
  }

  Task *TaskTextureLoadOGL::run(void) {
    Ref<Texture2D> tex = NULL;

    // The data is not needed anymore
    PF_DELETE(this->data);

    // Update the map to say we are done with this texture. We can now use it
    Lock<MutexSys> lock(streamer.mutex);
    auto it = streamer.texMap.find(name);
    assert(it != streamer.texMap.end());
    it->second.tex = tex;
    it->second.value = TextureState::COMPLETE;

    return NULL;
  }

  Ref<Task> TextureStreamer::loadTexture(const char *name) {
    Lock<MutexSys> lock(mutex);

    // Somebody already issued a load for it
    TextureState texState = this->getTextureStateUnlocked(name);
    if (texState.value == TextureState::LOADING ||
        texState.value == TextureState::COMPLETE)
      return texState.loadingTask;

    // Create the task and indicate everybody else that texture is loading
    Task *loadingTask = PF_NEW(TaskTextureLoad, name, *this);
    this->texMap[name] = TextureState(TextureState::LOADING, loadingTask);
    loadingTask->scheduled();
    return loadingTask;
  }
}
#undef OGL_NAME


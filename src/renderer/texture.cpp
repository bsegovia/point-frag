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

#include "renderer/texture.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderer_driver.hpp"
#include "sys/tasking.hpp"
#include "sys/logging.hpp"
#include "image/stb_image.hpp"
#include "image/squish/squish.h"
#include "math/math.hpp"

#include <cstring>
#include <algorithm>

#define OGL_NAME (renderer.driver)
namespace pf
{
  static unsigned char *doMipmap(const unsigned char *src,
                                 int w, int h,
                                 int mmW, int mmH,
                                 int channelNum)
  {
    assert(src && w && h && mmW && mmH && channelNum);
    unsigned char *dst = (unsigned char*) PF_MALLOC(mmW*mmH*channelNum);
    for (int y = 0; y < mmH; ++y) {
      for (int x = 0; x < mmW; ++x) {
        const int offset = (x + y*mmW) * channelNum;
        const int offset0 = (2*x+0 + 2*y*w+0) * channelNum;
        const int offset1 = (2*x+1 + 2*y*w+w) * channelNum;
        const int offset2 = (2*x+0 + 2*y*w+w) * channelNum;
        const int offset3 = (2*x+1 + 2*y*w+0) * channelNum;
        for (int c = 0; c < channelNum; ++c) {
          const float f = float(src[offset0+c]) +
                          float(src[offset1+c]) +
                          float(src[offset2+c]) +
                          float(src[offset3+c]);
          dst[offset+c] = (unsigned char) (f * 0.25f);
        }
      }
    }
    return dst;
  }

  static void mirror(unsigned char *img, int w, int h, int channelNum)
  {
    assert(img != NULL);
    for (int y = 0; y < h / 2; ++y)
      for (int x = 0; x < w; ++x) {
        const int offset = channelNum * (y*w + x);
        const int mirror = channelNum * ((h-y-1)*w + x);
        for (int c = 0; c < channelNum; ++c)
          std::swap(img[offset+c], img[mirror+c]);
      }
  }

  Texture2D::Texture2D(Renderer &renderer) :
    renderer(renderer), handle(0), fmt(0), w(0), h(0), minLevel(0), maxLevel(0)
  {}

  Texture2D::~Texture2D(void) {
    if (this->handle != 0) R_CALL (DeleteTextures, 1, &this->handle);
  }

  TextureState TextureStreamer::getTextureState(const std::string &name) {
    Lock<MutexSys> lock(mutex);
    return this->getTextureStateUnlocked(name);
  }

  TextureState TextureStreamer::getTextureStateUnlocked(const std::string &name) {
    auto it = texMap.find(name);
    if (it == texMap.end())
      return TextureState();
    else
      return it->second;
  }

#define DECL_SWITCH_CASE(CASE) case CASE: return #CASE
  static INLINE const char *TextureGetDXTName(TextureFormat fmt) {
    switch (fmt) {
      DECL_SWITCH_CASE(PF_TEX_FORMAT_PLAIN);
      DECL_SWITCH_CASE(PF_TEX_FORMAT_DXT1);
      DECL_SWITCH_CASE(PF_TEX_FORMAT_DXT3);
      DECL_SWITCH_CASE(PF_TEX_FORMAT_DXT5);
    };
    return "";
  }

  static INLINE const char *TextureGetDXTQuality(TextureQuality quality) {
    switch (quality) {
      DECL_SWITCH_CASE(PF_TEX_DXT_LOW_QUALITY);
      DECL_SWITCH_CASE(PF_TEX_DXT_NORMAL_QUALITY);
      DECL_SWITCH_CASE(PF_TEX_DXT_HIGH_QUALITY);
    };
    return "";
  }
#undef DECL_SWITCH_CASE

  /*! Contain the texture data (format + data) that we will provide to OGL */
  class TextureLoadData
  {
  public:
    TextureLoadData(const TextureRequest &request);
    ~TextureLoadData(void);
    INLINE bool isValid(void) const { return texels != NULL; }
    TextureRequest request;
    unsigned char **texels; //!< All mip-map level texels
    int *w, *h;             //!< Dimensions of all mip-maps
    size_t *sz;             //!< Size of them (if compression used)
    int levelNum;           //!< Number of mip-maps
    int fmt;                //!< Format of the texture
  };

  TextureLoadData::TextureLoadData(const TextureRequest &request) :
    request(request),
    texels(NULL), w(NULL), h(NULL), sz(NULL),
    levelNum(0), fmt(0)
  {

    // Try to find the file and load it
    bool isLoaded = false;
    unsigned char *img = NULL;
    int w0 = 0, h0 = 0, channel = 0;

    // We only force 4 channels for compression (squish requires it)
    const int reqComp = request.fmt == PF_TEX_FORMAT_PLAIN ? 0 : 4;
    for (size_t i = 0; i < defaultPathNum; ++i) {
      const FileName dataPath(defaultPath[i]);
      const FileName path = dataPath + FileName(request.name);
      img = stbi_load(path.c_str(), &w0, &h0, &channel, reqComp);
      if (img == NULL)
        continue;
      else {
        isLoaded = true;
        break;
      }
    }

    // We forced RGBA only while using DXT compression
    channel = request.fmt == PF_TEX_FORMAT_PLAIN ? channel : reqComp;

    // We found the image
    if (isLoaded) {
      mirror(img, w0, h0, channel);
      if (request.minFilter != GL_LINEAR_MIPMAP_LINEAR  &&
          request.minFilter != GL_LINEAR_MIPMAP_NEAREST &&
          request.minFilter != GL_NEAREST_MIPMAP_LINEAR &&
          request.minFilter != GL_NEAREST_MIPMAP_NEAREST)
        this->levelNum = 0;
      else
        this->levelNum = (int) max(log2(float(w0)), log2(float(h0)));
      this->w = PF_NEW_ARRAY(int, this->levelNum+1);
      this->h = PF_NEW_ARRAY(int, this->levelNum+1);
      this->sz = PF_NEW_ARRAY(size_t, this->levelNum+1);
      this->texels = PF_NEW_ARRAY(unsigned char*, this->levelNum+1);
      this->texels[0] = img;
      this->w[0] = w0;
      this->h[0] = h0;
      this->sz[0] = 0; // non zero only if compressed

      // Now compute the mip-maps
      for (int lvl = 1; lvl <= levelNum; ++lvl) {
        const int mmW = max(w0 / 2, 1), mmH = max(h0 / 2, 1);
        unsigned char *mipmap = doMipmap(img, w0, h0, mmW, mmH, channel);
        this->texels[lvl] = mipmap;
        this->w[lvl] = mmW;
        this->h[lvl] = mmH;
        this->sz[lvl] = 0; // non zero only if compressed
        w0 = mmW;
        h0 = mmH;
        img = mipmap;
      }

      if (request.fmt == PF_TEX_FORMAT_PLAIN) {
        switch (channel) {
          case 3: this->fmt = GL_RGB;  break;
          case 4: this->fmt = GL_RGBA; break;
          default: FATAL("Unsupported number of channels");
        };
      }
      else {
        using namespace squish;
        PF_MSG_V("TextureStreamer: compressing: " << request.name << ", " <<
                  TextureGetDXTName(request.fmt) << ", " <<
                  TextureGetDXTQuality(request.quality));
        // Get DXT format
        int squishFmt = 0;
        this->fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        switch (request.fmt) {
          case PF_TEX_FORMAT_DXT3:
            this->fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            squishFmt = kDxt3;
          break;
          case PF_TEX_FORMAT_DXT5:
            this->fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            squishFmt = kDxt5;
          break;
          default:
            this->fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            squishFmt = kDxt1;
          break;
        };

        // Get compression quality
        int squishQuality = 0;
        switch (request.quality) {
          case PF_TEX_DXT_NORMAL_QUALITY:
            squishQuality = kColourClusterFit;
          break;
          case PF_TEX_DXT_LOW_QUALITY:
            squishFmt = kColourRangeFit;
          break;
          default:
            squishFmt = kColourIterativeClusterFit;
          break;
        };

        // Compress all mip-maps
        const int flags = squishFmt | squishQuality;
        for (int lvl = 0; lvl <= levelNum; ++lvl) {
          this->sz[lvl] = GetStorageRequirements(w[lvl], h[lvl], flags);
          u8 *compressed = (u8 *) PF_MALLOC(this->sz[lvl]);
          CompressImage(texels[lvl], w[lvl], h[lvl], compressed, flags);
          PF_FREE(texels[lvl]);
          this->texels[lvl] = compressed;
        }
      }
    }
  }

  TextureLoadData::~TextureLoadData(void)
  {
    PF_SAFE_DELETE_ARRAY(this->w);
    PF_SAFE_DELETE_ARRAY(this->h);
    PF_SAFE_DELETE_ARRAY(this->sz);
    if (this->texels)
      for (int i = 0; i <= this->levelNum; ++i)
        PF_FREE(this->texels[i]);
    PF_SAFE_DELETE_ARRAY(this->texels);
  }

  class TaskTextureLoad;    //!< Load the textures from the disk
  class TaskTextureLoadOGL; //!< Upload the mip level to OGL

  /*! First task simply loads the texture from memory and build all the
   *  mip-maps. We store everything in the texture load data
   */
  class TaskTextureLoad : public Task
  {
  public:
    INLINE TaskTextureLoad(const TextureRequest &request, TextureStreamer &streamer) :
      Task("TaskTextureLoad"), request(request), streamer(streamer)
    {
      this->setPriority(TaskPriority::LOW);
    }
    virtual Task* run(void);
    TextureRequest request;    //!< File to load
    TextureStreamer &streamer; //!< Streamer that handles streaming
  };

  /*! Second task finishes the work by creating the OGL texture */
  class TaskTextureLoadOGL : public TaskMain
  {
  public:
    INLINE TaskTextureLoadOGL(TextureLoadData *data, TextureStreamer &streamer) :
      TaskMain("TaskTextureLoadOGL"), data(data), streamer(streamer)
    {
      assert(data != NULL &&
             data->w != NULL && data->h != NULL &&
             data->texels != NULL);
      this->setPriority(TaskPriority::HIGH);
    }
    virtual Task* run(void);
    TextureLoadData *data;     //!< Data to upload
    TextureStreamer &streamer; //!< Streamer that handles the texture
  };

  Task *TaskTextureLoad::run(void) {
    PF_MSG_V("TextureStreamer: loading: " << request.name);
    double t = getSeconds();
    TextureLoadData *data = PF_NEW(TextureLoadData, request);

    // We were not able to find the texture. So we use a default one
    if (data->isValid() == false) {
      PF_MSG_V("TextureStreamer: texture: " << request.name << " not found");
      PF_DELETE(data);
      Lock<MutexSys> lock(streamer.mutex);
      assert(streamer.renderer.defaultTex);
      streamer.texMap[request.name] = TextureState(*streamer.renderer.defaultTex);
    }
    // We need to load it in OGL now
    else {
      PF_MSG_V("TextureStreamer: loading time: " << request.name <<
               ", " << getSeconds() - t << "s");
      Task *next = PF_NEW(TaskTextureLoadOGL, data, streamer);
      next->ends(this);
      next->scheduled();
    }

    return NULL;
  }

  /*! When a concurrent load is done on the *same* resource, we simply create
   * a proxy task that waits for the loading task itself
   */
  class TaskTextureLoadProxy : public Task
  {
  public:
    TaskTextureLoadProxy(Ref<Task> loadingTask) :
      Task("TaskTextureLoadProxy"), loadingTask(loadingTask)
    { assert(this->loadingTask); }
    virtual Task *run(void) {
      loadingTask->waitForCompletion();
      return NULL;
    }
    Ref<Task> loadingTask;
  };

#undef OGL_NAME
#define OGL_NAME (this->streamer.renderer.driver)

  Task *TaskTextureLoadOGL::run(void)
  {
    const TextureRequest &request = data->request;
    PF_MSG_V("TextureStreamer: OGL uploading: " << request.name);
    double t = getSeconds();
    Ref<Texture2D> tex = PF_NEW(Texture2D, streamer.renderer);
    tex->w = data->w[0];
    tex->h = data->h[0];
    tex->fmt = data->fmt;
    tex->minLevel = 0;
    tex->maxLevel = data->levelNum;

    // Create the texture and its mip-maps
    R_CALL (GenTextures, 1, &tex->handle);
    R_CALL (ActiveTexture, GL_TEXTURE0);
    R_CALL (BindTexture, GL_TEXTURE_2D, tex->handle);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, request.minFilter);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, request.magFilter);

    // Upload the mip-maps
    if (request.fmt == PF_TEX_FORMAT_PLAIN)
      for (GLuint lvl = tex->minLevel; lvl <= tex->maxLevel; ++lvl) {
          R_CALL (TexImage2D,
                  GL_TEXTURE_2D,
                  lvl,
                  tex->fmt, data->w[lvl], data->h[lvl],
                  0,
                  tex->fmt,
                  GL_UNSIGNED_BYTE,
                  data->texels[lvl]);
      }
    else 
      for (GLuint lvl = tex->minLevel; lvl <= tex->maxLevel; ++lvl) {
          R_CALL (CompressedTexImage2D,
                  GL_TEXTURE_2D,
                  lvl,
                  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                  data->w[lvl], data->h[lvl],
                  0,
                  data->sz[lvl],
                  data->texels[lvl]);

        }

    // Update the map to say we are done with this texture. We can now use it
    Lock<MutexSys> lock(streamer.mutex);
    auto it = streamer.texMap.find(request.name);
    assert(it != streamer.texMap.end());
    it->second.loadingTask = NULL;
    it->second.tex = tex;
    it->second.value = TextureState::COMPLETE;
    PF_MSG_V("TextureStreamer: OGL uploading time: " <<
             request.name << ", " << getSeconds() - t << "s");

    // The data is not needed anymore
    PF_DELETE(this->data);

    return NULL;
  }
#undef OGL_NAME

  TextureStreamer::TextureStreamer(Renderer &renderer) : renderer(renderer) {}
  TextureStreamer::~TextureStreamer(void) {
    // Properly for all outstanding tasks
    for (auto it = texMap.begin(); it != texMap.end(); ++it) {
      TextureState &state = it->second;
      if (state.loadingTask)
        state.loadingTask->waitForCompletion();
    }
  }

  Ref<Task> TextureStreamer::createLoadTask(const TextureRequest &request)
  {
    Lock<MutexSys> lock(mutex);

    // Somebody already issued a load for it
    TextureState texState = this->getTextureStateUnlocked(request.name);
    if (texState.value == TextureState::LOADING ||
        texState.value == TextureState::COMPLETE) {
      if (texState.loadingTask == false)
        return NULL;
      else
        return PF_NEW(TaskTextureLoadProxy, texState.loadingTask);
    }

    // Create the task and indicate everybody else that texture is loading
    Task *loadingTask = PF_NEW(TaskTextureLoad, request, *this);
    this->texMap[request.name] = TextureState(TextureState::LOADING, loadingTask);
    return loadingTask;
  }

} /* namespace pf */


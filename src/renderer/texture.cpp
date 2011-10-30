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
#include "math/math.hpp"

#include <cstring>
#include <algorithm>

#define OGL_NAME (renderer.driver)
namespace pf
{
  static unsigned char *doMipmap(const unsigned char *src,
                                 int w, int h,
                                 int mmW, int mmH,
                                 int compNum)
  {
    unsigned char *dst = (unsigned char*) PF_MALLOC(mmW*mmH*compNum);
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

  static void mirror(unsigned char *img, int w, int h, int compNum)
  {
    for (int y = 0; y < h / 2; ++y)
    for (int x = 0; x < w; ++x) {
      const int offset = compNum * (y*w + x);
      const int mirror = compNum * ((h-y-1)*w + x);
      for (int c = 0; c < compNum; ++c)
        std::swap(img[offset+c], img[mirror+c]);
    }
  }

  Texture2D::Texture2D(Renderer &renderer) :
    renderer(renderer), handle(0), fmt(0), w(0), h(0), minLevel(0), maxLevel(0)
  {}

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
    TextureLoadData(const std::string &name);
    ~TextureLoadData(void);
    INLINE bool isValid(void) const { return texels != NULL; }
    unsigned char **texels; //!< All mip-map level texels
    int *w, *h;             //!< Dimensions of all mip-maps
    std::string name;       //!< Name of the file containing the data
    int levelNum;           //!< Number of mip-maps
    int fmt;                //!< Format of the textures
  };

  TextureLoadData::TextureLoadData(const std::string &name) :
    texels(NULL), w(NULL), h(NULL), name(name), levelNum(0)
  {
    // Try to find the file and load it
    bool isLoaded = false;
    unsigned char *img = 0;
    int w0 = 0, h0 = 0, channel = 0;
    for (size_t i = 0; i < defaultPathNum; ++i) {
      const FileName dataPath(defaultPath[i]);
      const FileName path = dataPath + FileName(name);
      img = stbi_load(path.c_str(), &w0, &h0, &channel, 0);
      if (img == NULL)
        continue;
      else {
        isLoaded = true;
        break;
      }
    }

    // We found the image
    if (isLoaded) {
      mirror(img, w0, h0, channel);
      this->levelNum = (int) max(log2(float(w0)), log2(float(h0)));
      this->w = PF_NEW_ARRAY(int, this->levelNum+1);
      this->h = PF_NEW_ARRAY(int, this->levelNum+1);
      this->texels = PF_NEW_ARRAY(unsigned char*, this->levelNum+1);
      this->texels[0] = img;
      this->w[0] = w0;
      this->h[0] = h0;
      switch (channel) {
        case 3: this->fmt = GL_RGB; break;
        case 4: this->fmt = GL_RGBA; break;
        default: FATAL("unsupported number of componenents");
      };
      // Now compute the mip-maps
      for (int lvl = 1; lvl <= levelNum; ++lvl) {
        const int mmW = max(w0 / 2, 1), mmH = max(h0 / 2, 1);
        unsigned char *mipmap = doMipmap(img, w0, h0, mmW, mmH, channel);
        this->texels[lvl] = mipmap;
        this->w[lvl] = mmW;
        this->h[lvl] = mmH;
        w0 = mmW;
        h0 = mmH;
        img = mipmap;
      }
    }
  }

  TextureLoadData::~TextureLoadData(void)
  {
    PF_DELETE_ARRAY(this->w);
    PF_DELETE_ARRAY(this->h);
    for (int i = 0; i <= this->levelNum; ++i) PF_FREE(this->texels[i]);
    PF_DELETE_ARRAY(this->texels);
  }

  class TaskTextureLoad;    //!< Load the textures from the disk
  class TaskTextureLoadOGL; //!< Upload the mip level to OGL

  /*! First task simply loads the texture from memory and build all the
   *  mip-maps. We store everything in the texture load data
   */
  class TaskTextureLoad : public Task
  {
  public:
    INLINE TaskTextureLoad(const std::string name, TextureStreamer &streamer) :
      Task("TaskTextureLoad"), name(name), streamer(streamer)
    {
      this->setPriority(TaskPriority::LOW);
    }
    virtual Task* run(void);
    std::string name;          //!< File to load
    TextureStreamer &streamer; //!< Streamer that handles streaming
  };

  /*! Second task finishes the work by creating the OGL texture */
  class TaskTextureLoadOGL : public TaskMain
  {
  public:
    INLINE TaskTextureLoadOGL(TextureLoadData *data, TextureStreamer &streamer) :
      TaskMain("TaskTextureLoadOGL"), data(data), streamer(streamer)
    {
      this->setPriority(TaskPriority::LOW);
    }
    virtual Task* run(void);
    TextureLoadData *data;     //!< Data to upload
    TextureStreamer &streamer; //!< Streamer that handles the texture
  };

  Task *TaskTextureLoad::run(void) {
    PF_MSG_V("TextureStreamer: loading " << name);
    TextureLoadData *data = PF_NEW(TextureLoadData, this->name);

    // We were not able to find the texture. So we use a default one
    if (data->isValid() == false) {
      PF_MSG_V("TextureStreamer: Texture " << std::string(name) <<
               " not found. Default texture is used instead");
      PF_DELETE(data);
      Lock<MutexSys> lock(streamer.mutex);
      streamer.texMap[name] = TextureState(*streamer.renderer.defaultTex);
    }
    // We need to load it in OGL now
    else {
      Task *next = PF_NEW(TaskTextureLoadOGL, data, this->streamer);
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
      Task("TaskTextureLoadProxy"), loadingTask(loadingTask) {}
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
    PF_MSG_V("TextureStreamer: OGL uploading " << data->name);
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
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (GLuint lvl = tex->minLevel; lvl <= tex->maxLevel; ++lvl)
      R_CALL (TexImage2D,
              GL_TEXTURE_2D,
              lvl,
              tex->fmt, data->w[lvl], data->h[lvl],
              0,
              tex->fmt,
              GL_UNSIGNED_BYTE,
              data->texels[lvl]);

    // Update the map to say we are done with this texture. We can now use it
    Lock<MutexSys> lock(streamer.mutex);
    auto it = streamer.texMap.find(this->data->name);
    assert(it != streamer.texMap.end());
    it->second.loadingTask = NULL;
    it->second.tex = tex;
    it->second.value = TextureState::COMPLETE;

    // The data is not needed anymore
    PF_DELETE(this->data);

    PF_MSG_V("TextureStreamer: uploading " << data->name << 
             " time: " << (getSeconds() - t) << "s");

    return NULL;
  }
#undef OGL_NAME

  TextureStreamer::TextureStreamer(Renderer &renderer) : renderer(renderer) {}
  TextureStreamer::~TextureStreamer(void) {
    for (auto it = texMap.begin(); it != texMap.end(); ++it) {
      TextureState &state = it->second;
      if (state.loadingTask)
        state.loadingTask->waitForCompletion();
    }
  }

  Ref<Task> TextureStreamer::createLoadTask(const FileName &name)
  {
    Lock<MutexSys> lock(mutex);

    // Somebody already issued a load for it
    TextureState texState = this->getTextureStateUnlocked(name.c_str());
    if (texState.value == TextureState::LOADING ||
        texState.value == TextureState::COMPLETE) {
      if (texState.loadingTask == false)
        return NULL;
      else
        return PF_NEW(TaskTextureLoadProxy, texState.loadingTask);
    }

    // Create the task and indicate everybody else that texture is loading
    Task *loadingTask = PF_NEW(TaskTextureLoad, name.c_str(), *this);
    this->texMap[name.str()] = TextureState(TextureState::LOADING, loadingTask);
    return loadingTask;
  }

} /* namespace pf */


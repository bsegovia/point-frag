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

#ifndef __PF_TEXTURE_HPP__
#define __PF_TEXTURE_HPP__

#include "sys/filename.hpp"
#include "sys/ref.hpp"
#include "sys/mutex.hpp"
#include "sys/tasking.hpp"
#include "renderer/GL/gl3.h"
#ifdef __MSVC__
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

namespace pf
{
  /*! Its owns all the textures */
  class Renderer;

  /*! Small wrapper around a GL 2D texture */
  struct Texture2D : RefCount
  {
    /*! Create an empty texture */
    Texture2D(Renderer &renderer);
    /*! Release it from OGL */
    ~Texture2D(void);
    /*! Valid means it is actually in GL */
    INLINE bool isValid(void) const {return this->handle != 0;}
    Renderer &renderer; //!< Creates this class
    GLuint handle;      //!< Texture object itself
    GLuint fmt;         //!< GL format of the texture
    GLuint w, h;        //!< Dimensions of level 0
    GLuint minLevel;    //!< Minimum level we loaded
    GLuint maxLevel;    //!< Maximum level we loaded
    GLuint magFilter;   //!< Magnification filter
    GLuint minFilter;   //!< Magnification filter
    GLuint internalFmt; //!< Internal format
  };

  /*! Texture compression quality when used */
  enum TextureQuality
  {
    PF_TEX_DXT_HIGH_QUALITY = 0,
    PF_TEX_DXT_NORMAL_QUALITY = 1,
    PF_TEX_DXT_LOW_QUALITY = 2
  };

  /*! Format we want OGL wise */
  enum TextureFormat
  {
    PF_TEX_FORMAT_PLAIN = 0,
    PF_TEX_FORMAT_DXT1  = 1,
    PF_TEX_FORMAT_DXT3  = 2,
    PF_TEX_FORMAT_DXT5  = 3
  };

  /*! Describe how to load a texture and what to do with it */
  struct TextureRequest
  {
    INLINE TextureRequest(const std::string &name,
                          TextureFormat fmt,
                          GLuint minFilter = GL_LINEAR_MIPMAP_LINEAR,
                          GLuint magFilter = GL_LINEAR,
                          TextureQuality quality = PF_TEX_DXT_LOW_QUALITY) :
      name(name), fmt(fmt),
      minFilter(minFilter), magFilter(magFilter),
      quality(quality)  {}
    std::string name;       //!< Only if load from file
    TextureFormat fmt;      //!< Format we want in OGL
    GLuint minFilter;       //!< Minification
    GLuint magFilter;       //!< Magnification (will compute mip-maps if needed)
    TextureQuality quality; //!< Compression quality when used
  };

  /*! States of the texture. This also includes the loading task itself. This
   *  will allow any other tasks (including tasks that did not issue the load) to
   *  wait for it if required
   */
  struct TextureState
  {
    INLINE TextureState(void) : value(NOT_HERE) {}
    INLINE TextureState(Texture2D &tex) : tex(&tex), value(COMPLETE) {}
    TextureState(int value, Task &loadingTask, Task &proxyTask);
    Ref<Texture2D> tex;    //!< Texture itself
    Ref<Task> loadingTask; //!< Task that issued the load
    Ref<Task> proxyTask;   //!< Task that depends on the loading task (for sync)
    int value;             //!< Current loading state
    enum
    {
      NOT_HERE  = 0, //!< Unknown not in the streamer at all
      LOADING   = 1, //!< Somebody started to load it
      COMPLETE  = 2, //!< It is here!
      INVALID_STATE = 0xffffffff
    };
  };

  /*! The texture streamer is responsible of loading asynchronously the
   *  textures. This is the centralized place to decide what is going on when
   *  loading many textures possibly at the same time from many threads.
   *  It also stores the textures themselves
   */
  class TextureStreamer : public NonCopyable
  {
  public:
    TextureStreamer(Renderer &renderer);
    ~TextureStreamer(void);

    /*! Indicate the current loading state of the texture */
    TextureState getTextureState(const std::string &name);
    /*! If the texture is not loading, spawn a task and load it. Otherwise,
     *  return the task that is currently loading it. That may be NULL if the
     *  task is already loaded
     */
    Ref<Task> createLoadTask(const TextureRequest &request);

  private:
    /*! Store for each texture its state */
    std::tr1::unordered_map<std::string, TextureState> texMap;
    /*! Serialize the streamer access */
    MutexSys mutex;
    friend class TaskTextureLoad;    //!< Load the textures from the disk
    friend class TaskTextureLoadOGL; //!< Upload the mip level to OGL
    Renderer &renderer;              //!< Owner of the streamer
  };
} /* namespace pf */

#endif /* __PF_TEXTURE_HPP__ */


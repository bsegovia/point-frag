#ifndef __PF_TEXTURE_HPP__
#define __PF_TEXTURE_HPP__

#include "sys/filename.hpp"
#include "sys/ref.hpp"
#include "sys/mutex.hpp"
#include "sys/tasking.hpp"
#include "renderer/GL/gl3.h"

#include <unordered_map>

namespace pf
{
  /*! Its owns all the textures (TODO Change that to Renderer) */
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
    RendererDriver &renderer; //!< Creates this class
    GLuint handle;            //!< Texture object itself
    GLuint fmt;               //!< GL format of the texture
    GLuint w, h;              //!< Dimensions of level 0
    GLuint minLevel;          //!< Minimum level we loaded
    GLuint maxLevel;          //!< Maximum level we loaded
  };

  /*! States of the texture. This also includes the loading task itself. This
   *  will allow any other tasks (including tasks that did not issue the task to
   *  wait for it if required
   */
  struct TextureState
  {
    INLINE TextureState(void) : loadingTask(NULL), state(NOT_HERE) {}
    Ref<Task> loadingTask; //!< Pointer to the task that issued the load
    int state;             //!< Current loading state
    enum {
      NOT_HERE = 0, //!< Unknown not in the streamer at all
      LOADING = 1,  //!< Somebody started to load it
      COMPLETE = 2, //!< It is here!
      INVALID_STATE = 0xffffffff
    };
  };

  /*! The texture streamer is responsible of loading asynchronously the
   *  textures. This is the centralized place to decide what is going on when
   *  loading many textures possibly at the same time from many threads.
   *  It also *  stores the textures themselves
   */
  class TextureStreamer
  {
  public:
    TextureStreamer(void);
    ~TextureStreamer(void);

    /*! Indicate the current loading state of the texture */
    TextureState getTextureState(const char *name) const;

    /*! If the texture is not loading, spawn a task and load it. Otherwise,
     *  return the task that is currently loading it
     */
    Ref<Task> loadTexture(const char *name);

  private:
    /*! Sort the texture by name (only its base name is taken into account) */
    std::unordered_map<std::string, Ref<Texture2D>> texMap;
    MutexActive texMapMutex;  //!< Serializes texMap accesses

    /*! Store for each texture its state */
    std::unordered_map<std::string, TextureState> texState;
    MutexActive texStateMutex; //!< Serializes texState accesses
  };
}

#endif /* __PF_TEXTURE_HPP__ */


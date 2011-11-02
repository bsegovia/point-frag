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
  };

  /*! States of the texture. This also includes the loading task itself. This
   *  will allow any other tasks (including tasks that did not issue the load) to
   *  wait for it if required
   */
  struct TextureState
  {
    INLINE TextureState(void) : value(NOT_HERE) {}
    INLINE TextureState(int value, Task *task) :
      loadingTask(task), value(value) {}
    INLINE TextureState(Texture2D &tex) : tex(&tex), value(COMPLETE) {}
    Ref<Texture2D> tex;    //!< Texture itself
    Ref<Task> loadingTask; //!< Pointer to the task that issued the load
    int value;             //!< Current loading state
    enum {
      NOT_HERE = 0,  //!< Unknown not in the streamer at all
      LOADING = 1,   //!< Somebody started to load it
      COMPLETE = 2,  //!< It is here!
      NOT_FOUND = 3, //!< The resource was not found
      INVALID_STATE = 0xffffffff
    };
  };

  /*! The texture streamer is responsible of loading asynchronously the
   *  textures. This is the centralized place to decide what is going on when
   *  loading many textures possibly at the same time from many threads.
   *  It also stores the textures themselves
   */
  class TextureStreamer
  {
  public:
    TextureStreamer(Renderer &renderer);
    ~TextureStreamer(void);

    /*! Indicate the current loading state of the texture */
    TextureState getTextureState(const char *name);
    /*! If the texture is not loading, spawn a task and load it. Otherwise,
     *  return the task that is currently loading it. That may be NULL if the
     *  task is already loaded
     */
    Ref<Task> createLoadTask(const std::string &name);

  private:
    /*! Store for each texture its state */
    std::unordered_map<std::string, TextureState> texMap;
    /*! Serialize the streamer access */
    MutexSys mutex;
    /*! Unlocked for internal use */
    TextureState getTextureStateUnlocked(const std::string &name);
    friend class TaskTextureLoad;    //!< Load the textures from the disk
    friend class TaskTextureLoadOGL; //!< Upload the mip level to OGL
    Renderer &renderer;              //!< Owner of the streamer
  };
} /* namespace pf */

#endif /* __PF_TEXTURE_HPP__ */


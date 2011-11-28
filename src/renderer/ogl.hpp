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

#ifndef __PF_OGL_HPP__
#define __PF_OGL_HPP__

#include "sys/platform.hpp"
#include "sys/filename.hpp"
#include "sys/atomic.hpp"
#include "sys/ref.hpp"

#include "renderer/GL/gl3.h"
#include "renderer/GL/glext.h"
#include <string>
#include <vector>

namespace pf
{
  // We use a task to track resource deallocation (see below)
  class Task;

  /*! We load all OGL functions explicitly */
  struct OGL
  {
    OGL(void);
    virtual ~OGL(void);

/*! We instantiate all GL functions here */
#define DECL_GL_PROC(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD;
#include "renderer/GL/ogl100.hxx"
#include "renderer/GL/ogl110.hxx"
#include "renderer/GL/ogl120.hxx"
#include "renderer/GL/ogl130.hxx"
#include "renderer/GL/ogl150.hxx"
#include "renderer/GL/ogl200.hxx"
#include "renderer/GL/ogl300.hxx"
#include "renderer/GL/ogl310.hxx"
#include "renderer/GL/ogl320.hxx"
#undef DECL_GL_PROC

    /*! We count all OGL allocations by overriding all OGL allocation and
     *  deletion functions
     */
    Atomic textureNum;
    Atomic vertexArrayNum;
    Atomic bufferNum;
    Atomic frameBufferNum;
    INLINE void GenTextures(GLsizei n, GLuint *tex) {
      this->textureNum += n;
      this->_GenTextures(n, tex);
    }
    INLINE void GenBuffers(GLsizei n, GLuint *buffer) {
      this->bufferNum += n;
      this->_GenBuffers(n, buffer);
    }
    INLINE void GenFramebuffers(GLsizei n, GLuint *buffer) {
      this->frameBufferNum += n;
      this->_GenFramebuffers(n, buffer);
    }
    INLINE void GenVertexArrays(GLsizei n, GLuint *buffer) {
      this->vertexArrayNum += n;
      this->_GenVertexArrays(n, buffer);
    }
    void DeleteTextures(GLsizei n, const GLuint *tex);
    void DeleteBuffers(GLsizei n, const GLuint *buffer);
    void DeleteFramebuffers(GLsizei n, const GLuint *buffer);
    void DeleteVertexArrays(GLsizei n, const GLuint *buffer);

    /*! Driver dependent constants */
    int32_t maxColorAttachmentNum;
    int32_t maxTextureSize;
    int32_t maxTextureUnit;

    /*! Swap back and front buffers */
    void swapBuffers(void) const;
    /*! Check that no error happened */
    bool checkError(const char *title = NULL) const;
    /*! Check that the frame buffer is properly setup */
    bool checkFramebuffer(GLuint frameBufferName) const;
    /*! Validate the program */
    bool validateProgram(GLuint programName) const;
    /*! Check that the program is properly created */
    bool checkProgram(GLuint programName) const;
    /*! Create a shader from the path of the source */
    GLuint createShader(GLenum type, const FileName &path) const;
    /*! Create a shader from the source */
    GLuint createShader(GLenum type, const std::string &source) const;

    /*! We delete resources asynchronously. Indeed since resource can be freed
     *  from anywhere, we use MAIN tasks to forward the destruction on the
     *  main thread. This task is just the continuation of all destruction
     *  tasks. This will allow us to wait for *all* destruction tasks by just
     *  doing scheduled and then waitForCompletion on it
     */
    Ref<Task> waitForDestruction;
  };

/*! Convenient way to catch all GL related errors. User defines the name of the
 *  structure which contains all OGL functions with name OGL_NAME
 */
#ifndef NDEBUG
  #define R_CALL(NAME, ...)                                                    \
    do {                                                                       \
      OGL_NAME->NAME(__VA_ARGS__);                                             \
      FATAL_IF (OGL_NAME->checkError(__FUNCTION__) == false, #NAME " failed"); \
    } while (0)
  #define R_CALLR(RET, NAME, ...)                                              \
    do {                                                                       \
      RET = OGL_NAME->NAME(__VA_ARGS__);                                       \
      FATAL_IF (OGL_NAME->checkError(__FUNCTION__) == false, #NAME " failed"); \
    } while (0)
#else
  #define R_CALL(NAME, ...)                                                    \
    do {                                                                       \
      OGL_NAME->NAME(__VA_ARGS__);                                             \
    } while (0)
  #define R_CALLR(RET, NAME, ...)                                              \
    do {                                                                       \
      RET = OGL_NAME->NAME(__VA_ARGS__);                                       \
    } while (0)
#endif /* NDEBUG */
}
#endif /* __PF_OGL_HPP__ */

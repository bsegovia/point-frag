#ifndef __OGL_HPP__
#define __OGL_HPP__

#include "sys/platform.hpp"
#include "sys/filename.hpp"
#include "sys/atomic.hpp"

#include "GL/gl3.h"
#include "GL/glext.h"
#include <string>
#include <vector>

namespace pf
{
  /*! We load all OGL functions explicitly */
  struct OGL {
    OGL(void);
    virtual ~OGL(void);

/*! We instantiate all GL functions here */
#define DECL_GL_PROC(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD;
#include "ogl100.hxx"
#include "ogl110.hxx"
#include "ogl120.hxx"
#include "ogl130.hxx"
#include "ogl150.hxx"
#include "ogl200.hxx"
#include "ogl300.hxx"
#include "ogl310.hxx"
#include "ogl320.hxx"
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
    INLINE void DeleteTextures(GLsizei n, const GLuint *tex) {
      this->textureNum += -n;
      this->_DeleteTextures(n, tex);
    }
    INLINE void GenBuffers(GLsizei n, GLuint *buffer) {
      this->bufferNum += n;
      this->_GenBuffers(n, buffer);
    }
    INLINE void DeleteBuffers(GLsizei n, const GLuint *buffer) {
      this->bufferNum += -n;
      this->_DeleteBuffers(n, buffer);
    }
    INLINE void GenFramebuffers(GLsizei n, GLuint *buffer) {
      this->frameBufferNum += n;
      this->_GenFramebuffers(n, buffer);
    }
    INLINE void DeleteFramebuffers(GLsizei n, const GLuint *buffer) {
      this->frameBufferNum += -n;
      this->_DeleteFramebuffers(n, buffer);
    }
    INLINE void GenVertexArrays(GLsizei n, GLuint *buffer) {
      this->vertexArrayNum += n;
      this->_GenVertexArrays(n, buffer);
    }
    INLINE void DeleteVertexArrays(GLsizei n, const GLuint *buffer) {
      this->vertexArrayNum += -n;
      this->_DeleteVertexArrays(n, buffer);
    }

    /*! Driver dependent constants */
    int32_t maxColorAttachmentNum;
    int32_t maxTextureSize;
    int32_t maxTextureUnit;

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
  };

  /*! One instance is enough */
  extern OGL *ogl;

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

#endif /* __OGL_HPP__ */


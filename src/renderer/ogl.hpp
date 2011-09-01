#ifndef __OGL_HPP__
#define __OGL_HPP__

#include "sys/platform.hpp"
#include "sys/filename.hpp"

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
  #define R_CALL(NAME, ...)                                                   \
    do {                                                                      \
      OGL_NAME->NAME(__VA_ARGS__);                                            \
      FATAL_IF (OGL_NAME->checkError(__FUNCTION__) == false, #NAME " failed");\
    } while (0)
  #define R_CALLR(RET, NAME, ...)                                             \
    do {                                                                      \
      RET = OGL_NAME->NAME(__VA_ARGS__);                                      \
      FATAL_IF (OGL_NAME->checkError(__FUNCTION__) == false, #NAME " failed");\
    } while (0)
#else
  #define R_CALL(NAME, ...) do { OGL_NAME->NAME(__VA_ARGS__); } while (0)
  #define R_CALLR(RET, NAME, ...) do { RET = OGL_NAME->NAME(__VA_ARGS__); } while (0)
#endif /* NDEBUG */

}

#endif /* __OGL_HPP__ */


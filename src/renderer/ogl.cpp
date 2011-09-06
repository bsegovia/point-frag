#include "ogl.hpp"
#include "sys/platform.hpp"
#include "sys/string.hpp"
#include "math/math.hpp"
#include "GL/freeglut.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace pf
{
  OGL::OGL() :
    textureNum(0),
    vertexArrayNum(0),
    bufferNum(0),
    frameBufferNum(0)
  {
// On Windows, we directly load from OpenGL up 1.2 functions
#if defined(__WIN32__)
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE) this->FIELD = NAME;
#else
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE)                  \
    this->FIELD = (PROTOTYPE) glutGetProcAddress(#NAME);      \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required");
#endif /* __WIN32__ */

#include "ogl100.hxx"
#include "ogl110.hxx"

// Now, we load everything with glut on Windows too
#if defined(__WIN32__)
  #undef DECL_GL_PROC
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE) do {             \
    this->FIELD = (PROTOTYPE) glutGetProcAddress(#NAME);      \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required"); \
  } while (0)
#endif /* __WIN32__ */

#include "ogl120.hxx"
#include "ogl130.hxx"
#include "ogl150.hxx"
#include "ogl200.hxx"
#include "ogl300.hxx"
#include "ogl310.hxx"
#include "ogl320.hxx"

#undef DECL_GL_PROC

// Get driver dependent constants
#define GET_CST(ENUM, FIELD)                        \
    this->GetIntegerv(ENUM, &this->FIELD);          \
    fprintf(stdout, #ENUM " == %i\n", this->FIELD);
    GET_CST(GL_MAX_COLOR_ATTACHMENTS, maxColorAttachmentNum);
    GET_CST(GL_MAX_TEXTURE_SIZE, maxTextureSize);
    GET_CST(GL_MAX_TEXTURE_IMAGE_UNITS, maxTextureUnit);
#undef GET_CST
  }

  OGL::~OGL() {
    assert(this->textureNum == 0 &&
           this->vertexArrayNum == 0 &&
           this->bufferNum == 0 &&
           this->frameBufferNum == 0);
  }

  bool OGL::checkError(const char *title) const
  {
    std::string errString;
    int err;

    if ((err = this->GetError()) == GL_NO_ERROR)
      return true;
#define MAKE_ERR_STRING(ENUM)       \
  case ENUM:                        \
    errString = #ENUM;              \
    break;
    switch (err) {
      MAKE_ERR_STRING(GL_INVALID_ENUM);
      MAKE_ERR_STRING(GL_INVALID_VALUE);
      MAKE_ERR_STRING(GL_INVALID_OPERATION);
      MAKE_ERR_STRING(GL_INVALID_FRAMEBUFFER_OPERATION);
      MAKE_ERR_STRING(GL_OUT_OF_MEMORY);
      default: errString = "GL_UNKNOWN";
    }
#undef MAKE_ERR_STRING
    fprintf(stderr, "OpenGL err(%s): %s\n", errString.c_str(), title ? title : "");
    return err == GL_NO_ERROR;
  }

  bool OGL::checkFramebuffer(GLuint frameBufferName) const
  {
    GLenum status = this->CheckFramebufferStatus(GL_FRAMEBUFFER);
#define MAKE_ERR(ENUM)                            \
  case ENUM:                                      \
    fprintf(stderr, "OpenGL Error(%s)\n", #ENUM); \
    break;
    switch (status)
    {
      MAKE_ERR(GL_FRAMEBUFFER_UNDEFINED);
      MAKE_ERR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
      MAKE_ERR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
      MAKE_ERR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
      MAKE_ERR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
      MAKE_ERR(GL_FRAMEBUFFER_UNSUPPORTED);
      MAKE_ERR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
      MAKE_ERR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
    }
#undef MAKE_ERR
    return status != GL_FRAMEBUFFER_COMPLETE;
  }

  bool OGL::validateProgram(GLuint programName) const
  {
    if(!programName)
      return false;

    this->ValidateProgram(programName);
    GLint result = GL_FALSE;
    this->GetProgramiv(programName, GL_VALIDATE_STATUS, &result);

    if (result == GL_FALSE) {
      int infoLogLength;
      fprintf(stderr, "Validate program\n");
      this->GetProgramiv(programName, GL_INFO_LOG_LENGTH, &infoLogLength);
      std::vector<char> Buffer(infoLogLength);
      this->GetProgramInfoLog(programName, infoLogLength, NULL, &Buffer[0]);
      fprintf(stderr, "%s\n", &Buffer[0]);
    }

    return result == GL_TRUE;
  }

  bool OGL::checkProgram(GLuint programName) const
  {
    int infoLogLength;
    GLint result = GL_FALSE;

    if(!programName)
      return false;

    this->GetProgramiv(programName, GL_LINK_STATUS, &result);
    fprintf(stderr, "Linking program\n");
    this->GetProgramiv(programName, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> Buffer(max(infoLogLength, int(1)));
    this->GetProgramInfoLog(programName, infoLogLength, NULL, &Buffer[0]);
    fprintf(stderr, "%s\n", &Buffer[0]);

    return result == GL_TRUE;
  }

  static bool checkShader(const struct OGL *o, GLuint shaderName, const std::string &source)
  {
    GLint result = GL_FALSE;
    int infoLogLength;

    if (!shaderName)
      return false;
    o->GetShaderiv(shaderName, GL_COMPILE_STATUS, &result);
    o->GetShaderiv(shaderName, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (result == GL_FALSE) {
      std::vector<char> Buffer(infoLogLength);
      o->GetShaderInfoLog(shaderName, infoLogLength, NULL, &Buffer[0]);
      fprintf(stderr, "Compiling shader\n%s...\n", source.c_str());
      fprintf(stderr, "%s\n", &Buffer[0]);
    }
    return result == GL_TRUE;
  }

  GLuint OGL::createShader(GLenum type, const FileName &path) const
  {
    const std::string source = loadFile(path);
    return this->createShader(type, source);
  }

  GLuint OGL::createShader(GLenum type, const std::string &source) const
  {
    GLuint name = 0;
    bool validated = true;
    if (source.empty() == true)
      return 0;
    else {
      const char *sourcePointer = source.c_str();
      name = this->CreateShader(type);
      this->ShaderSource(name, 1, &sourcePointer, NULL);
      this->CompileShader(name);
      validated = checkShader(this, name, source);
      FATAL_IF (validated == false, "shader not valid");
    }

    return name;
  }

  struct OGL *ogl = NULL;
}


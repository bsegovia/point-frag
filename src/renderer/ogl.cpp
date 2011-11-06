#include "ogl.hpp"
#include "sys/platform.hpp"
#include "sys/logging.hpp"
#include "sys/string.hpp"
#include "math/math.hpp"

#include <GL/freeglut.h>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace pf
{

#define OGL_NAME this

  OGL::OGL() :
    textureNum(0),
    vertexArrayNum(0),
    bufferNum(0),
    frameBufferNum(0)
  {
// On Windows, we directly load from OpenGL 1.1 functions
#if defined(__WIN32__)
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE) this->FIELD = NAME;
#else
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE)                  \
    this->FIELD = (PROTOTYPE) glutGetProcAddress(#NAME);      \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required");
#endif /* __WIN32__ */

#include "GL/ogl100.hxx"
#include "GL/ogl110.hxx"

// Now, we load everything with glut on Windows too
#if defined(__WIN32__)
  #undef DECL_GL_PROC
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE)                  \
    this->FIELD = (PROTOTYPE) glutGetProcAddress(#NAME);      \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required");
#endif /* __WIN32__ */

#include "GL/ogl120.hxx"
#include "GL/ogl130.hxx"
#include "GL/ogl150.hxx"
#include "GL/ogl200.hxx"
#include "GL/ogl300.hxx"
#include "GL/ogl310.hxx"
#include "GL/ogl320.hxx"

#undef DECL_GL_PROC

    // Print information about the current OGL driver
    const unsigned char *version = NULL;
    const unsigned char *vendor = NULL;
    const unsigned char *renderer = NULL;
    R_CALLR (version, GetString, GL_VERSION);
    R_CALLR (vendor, GetString, GL_VENDOR);
    R_CALLR (renderer, GetString, GL_RENDERER);
    PF_MSG ("OGL: " << version);
    PF_MSG ("OGL: " << vendor);
    PF_MSG ("OGL: " << renderer);
	R_CALL (PixelStorei, GL_UNPACK_ALIGNMENT, 1);

    // Check extensions we need
    if (glutExtensionSupported("GL_EXT_texture_compression_s3tc"))
      PF_MSG ("OGL: GL_EXT_texture_compression_s3tc supported");
    else
      FATAL ("GL_EXT_texture_compression_s3tc unsupported");

	// ATI Driver WORK AROUND error is emitted here
	while (this->GetError() != GL_NO_ERROR);
	
	// Get driver dependent constants
#define GET_CST(ENUM, FIELD)                        \
    this->GetIntegerv(ENUM, &this->FIELD);          \
    PF_MSG("OGL: " #ENUM << " == " << this->FIELD);
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

#undef OGL_NAME

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
    PF_ERROR_V("OGL: " << errString << "): " << (title ? title : ""));
    return err == GL_NO_ERROR;
  }

  bool OGL::checkFramebuffer(GLuint frameBufferName) const
  {
    GLenum status = this->CheckFramebufferStatus(GL_FRAMEBUFFER);
#define MAKE_ERR(ENUM)            \
  case ENUM:                      \
    PF_ERROR_V("OGL: " << #ENUM); \
    break;
    switch (status) {
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
      PF_ERROR_V("OGL: program validation failed\n");
      this->GetProgramiv(programName, GL_INFO_LOG_LENGTH, &infoLogLength);
      std::vector<char> buffer(infoLogLength);
      this->GetProgramInfoLog(programName, infoLogLength, NULL, &buffer[0]);
      PF_ERROR_V("OGL: " << &buffer[0]);
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
    if (result == GL_FALSE) {
      PF_ERROR_V("OGL: failed to link program");
      this->GetProgramiv(programName, GL_INFO_LOG_LENGTH, &infoLogLength);
      std::vector<char> buffer(max(infoLogLength, int(1)));
      this->GetProgramInfoLog(programName, infoLogLength, NULL, &buffer[0]);
      PF_ERROR_V("OGL: " << &buffer[0]);
    }

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
      std::vector<char> buffer(infoLogLength);
      o->GetShaderInfoLog(shaderName, infoLogLength, NULL, &buffer[0]);
      PF_ERROR_V("OGL: failed to compile shader\n" << source);
      PF_ERROR_V("OGL: " << &buffer[0]);
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
}


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

#include "ogl.hpp"
#include "sys/platform.hpp"
#include "sys/logging.hpp"
#include "sys/string.hpp"
#include "sys/tasking.hpp"
#include "sys/windowing.hpp"
#include "math/math.hpp"
#include "rt/ray.hpp"

#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace pf
{

#define OGL_NAME this

  /*! Just a dummy task to wait for the destruction tasks */
  class TaskWaitForOGLDestruction : public Task
  {
  public:
    TaskWaitForOGLDestruction(void) : Task("TaskWaitForOGLDestruction") {}
    virtual Task *run(void) { return NULL; }
  };

  OGL::OGL(void) :
    textureNum(0),
    vertexArrayNum(0),
    bufferNum(0),
    frameBufferNum(0)
  {
// On Windows, we directly load from OpenGL 1.1 functions
#if defined(__WIN32__)
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE) this->FIELD = (PROTOTYPE) NAME;
#else
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE)                  \
    this->FIELD = (PROTOTYPE) WinGetProcAddress(#NAME);       \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required");
#endif /* __WIN32__ */

#include "GL/ogl100.hxx"
#include "GL/ogl110.hxx"

// Now, we load everything with the window manager on Windows too
#if defined(__WIN32__)
  #undef DECL_GL_PROC
  #define DECL_GL_PROC(FIELD,NAME,PROTOTYPE)                  \
    this->FIELD = (PROTOTYPE) WinGetProcAddress(#NAME);       \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required");
#endif /* __WIN32__ */

#include "GL/ogl120.hxx"
#include "GL/ogl130.hxx"
#include "GL/ogl150.hxx"
#include "GL/ogl200.hxx"
#include "GL/ogl300.hxx"
//#include "GL/ogl310.hxx"
//#include "GL/ogl320.hxx"

#undef DECL_GL_PROC

    // Print information about the current OGL driver
    const unsigned char *version = NULL;
    const unsigned char *vendor = NULL;
    const unsigned char *renderer = NULL;
    const unsigned char *glsl = NULL;
    R_CALLR (version, GetString, GL_VERSION);
    R_CALLR (vendor, GetString, GL_VENDOR);
    R_CALLR (renderer, GetString, GL_RENDERER);
    R_CALLR (glsl, GetString, GL_SHADING_LANGUAGE_VERSION);
    PF_MSG ("OGL: " << version);
    PF_MSG ("OGL: " << vendor);
    PF_MSG ("OGL: " << renderer);
    PF_MSG ("OGL: " << glsl);
    FATAL_IF (atof((const char*) glsl) < 1.3f, "GLSL version 1.3 is required");
    R_CALL (PixelStorei, GL_UNPACK_ALIGNMENT, 1);

    // Check extensions we need
#define CHECK_EXTENSION(EXT)                                      \
    if (WinExtensionSupported(#EXT))                              \
      PF_MSG ("OGL: " #EXT " supported");                         \
    else                                                          \
      FATAL ("OGL: " #EXT " not supported");
    CHECK_EXTENSION(GL_EXT_texture_compression_s3tc);
    CHECK_EXTENSION(GL_ARB_texture_float);
    CHECK_EXTENSION(GL_ARB_sampler_objects);
    CHECK_EXTENSION(GL_ARB_vertex_buffer_object);
#undef CHECK_EXTENSION

    // ATI Driver WORK AROUND error is emitted here
    while (this->GetError() != GL_NO_ERROR);

// Get driver dependent constants
#define GET_CST(ENUM, FIELD)                        \
    this->GetIntegerv(ENUM, &this->FIELD);          \
    PF_MSG("OGL: " #ENUM << " == " << this->FIELD);
    GET_CST(GL_MAX_COLOR_ATTACHMENTS, maxColorAttachmentNum);
    GET_CST(GL_MAX_TEXTURE_SIZE, maxTextureSize);
    GET_CST(GL_MAX_TEXTURE_IMAGE_UNITS, maxTextureUnit);
    GET_CST(GL_MAX_RENDERBUFFER_SIZE, maxRenderBufferSize);
    GET_CST(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
    GET_CST(GL_MAX_3D_TEXTURE_SIZE, max3DTextureSize);
    GET_CST(GL_MAX_ELEMENTS_INDICES, maxElementIndices);
    GET_CST(GL_MAX_ELEMENTS_VERTICES, maxElementVertices);
    GET_CST(GL_MAX_CUBE_MAP_TEXTURE_SIZE, maxCubeMapTextureSize);
    GET_CST(GL_MAX_DRAW_BUFFERS, maxDrawBuffers);
    GET_CST(GL_MAX_VERTEX_ATTRIBS, maxVertexAttribs);
    GET_CST(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, maxFragmentUniformComponents);
    GET_CST(GL_MAX_VERTEX_UNIFORM_COMPONENTS, maxVertexUniformComponents);
    GET_CST(GL_MAX_VARYING_FLOATS, maxVaryingFloats);
    GET_CST(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, maxVertexTextureImageUnits);
    GET_CST(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, maxCombinedTextureImageUnits);
#undef GET_CST

    // Use that to wait for task destruction
    this->waitForDestruction = PF_NEW(TaskWaitForOGLDestruction);
  }

  OGL::~OGL(void)
  {
    this->waitForDestruction->scheduled();
    TaskingSystemWait(this->waitForDestruction);
    PF_ASSERT(this->textureNum == 0 &&
              this->vertexArrayNum == 0 &&
              this->bufferNum == 0 &&
              this->frameBufferNum == 0);
  }

#undef OGL_NAME

  void OGL::swapBuffers(void) const { WinSwapBuffers(); }
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
    PF_ERROR_V("OGL: " << errString << " " << (title ? title : ""));
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
      PF_ERROR_V("OGL: program validation failed");
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
    if (infoLogLength) {
      char *buffer = PF_NEW_ARRAY(char, infoLogLength + 1);
      buffer[infoLogLength] = 0;
      o->GetShaderInfoLog(shaderName, infoLogLength, NULL, buffer);
      PF_MSG_V("OGL: " << buffer);
      PF_DELETE_ARRAY(buffer);
    }
    if (result == GL_FALSE)
      PF_ERROR_V("OGL: failed to compile shader" << source);
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

  /*! Since only the main thread can destroy OGL resource and since the engine
   *  is completely multi-threaded and we may have reference counted objects
   *  destroyed by any thread, we chose a simple solution to handle OGL
   *  objects destructions: we forward it to the main thread with critical task.
   *  Since "destroyed" implies "unused", we do not care that it is
   *  asynchronous
   */
#define DECL_OGL_DESTROY_TASK(NAME, FUNC, COUNTER)                         \
  class TaskDestroy##NAME : public Task                                    \
  {                                                                        \
  public:                                                                  \
    /*! Store the data to destroy */                                       \
    TaskDestroy##NAME(OGL &ogl, const GLuint *handle_, GLuint n)           \
      : Task("TaskDestroy" #NAME),                                         \
        ogl(ogl),                                                          \
        handlePtr(NULL),                                                   \
        n(n),                                                              \
        deallocatePtr(false)                                               \
    {                                                                      \
      /* Fast path just uses the local storage of the task. Slow path */   \
      /* allocate an array to contain the handles */                       \
      if (UNLIKELY(n > OGL_MAX_HANDLE_NUM)) {                              \
        this->handlePtr = PF_NEW_ARRAY(GLuint, n);                         \
        deallocatePtr = true;                                              \
      } else                                                               \
        this->handlePtr = this->handle;                                    \
      std::memcpy(this->handlePtr, handle_, sizeof(GLuint) * n);           \
                                                                           \
      /* Go to MAIN only and as fast as possible */                        \
      this->setAffinity(PF_TASK_MAIN_THREAD);                              \
      this->setPriority(TaskPriority::CRITICAL);                           \
    }                                                                      \
                                                                           \
    /*! On the main thread, we really release the resource */              \
    virtual Task* run(void)                                                \
    {                                                                      \
      ogl.COUNTER += -this->n;                                             \
      ogl.FUNC(n, handlePtr);                                              \
      if (deallocatePtr) PF_DELETE_ARRAY(this->handlePtr);                 \
      return NULL;                                                         \
    }                                                                      \
                                                                           \
  private:                                                                 \
    static const uint32 OGL_MAX_HANDLE_NUM = 16;                           \
    OGL &ogl;                                                              \
    GLuint *handlePtr;                                                     \
    GLuint handle[OGL_MAX_HANDLE_NUM];                                     \
    int32 n;                                                               \
    bool deallocatePtr;                                                    \
  };

DECL_OGL_DESTROY_TASK(Texture, _DeleteTextures, textureNum);
DECL_OGL_DESTROY_TASK(Buffer, _DeleteBuffers, bufferNum);
DECL_OGL_DESTROY_TASK(Framebuffer, _DeleteFramebuffers, frameBufferNum);
DECL_OGL_DESTROY_TASK(VertexArray, _DeleteVertexArrays, vertexArrayNum);

#undef DECL_OGL_DESTROY_TASK

  void OGL::DeleteTextures(GLsizei n, const GLuint *tex) {
    Task *task = PF_NEW(TaskDestroyTexture, *this, tex, n);
    task->starts(this->waitForDestruction.ptr);
    task->scheduled();
  }
  void OGL::DeleteBuffers(GLsizei n, const GLuint *buffer) {
    Task *task = PF_NEW(TaskDestroyBuffer, *this, buffer, n);
    task->starts(this->waitForDestruction.ptr);
    task->scheduled();
  }
  void OGL::DeleteFramebuffers(GLsizei n, const GLuint *buffer) {
    Task *task = PF_NEW(TaskDestroyFramebuffer, *this, buffer, n);
    task->starts(this->waitForDestruction.ptr);
    task->scheduled();
  }
  void OGL::DeleteVertexArrays(GLsizei n, const GLuint *buffer) {
    Task *task = PF_NEW(TaskDestroyVertexArray, *this, buffer, n);
    task->starts(this->waitForDestruction.ptr);
    task->scheduled();
  }
} /* namespace pf */


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
  OGL::OGL()
  {
#if defined(__WIN32__)
  // On Windows, we directly load from OpenGL up 1.2 functions
  #define GET_PROC(FIELD,NAME,PROTOTYPE) this->FIELD = NAME
#else
  // Otherwise, we load everything manually
  #define GET_PROC(FIELD,NAME,PROTOTYPE) do {                 \
    this->FIELD = (PROTOTYPE) glutGetProcAddress(#NAME);      \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required"); \
  } while (0)
#endif /* __WIN32__ */

    // Load OpenGL 1.0 functions
    GET_PROC(CullFace, glCullFace, PFNGLCULLFACEPROC);
    GET_PROC(FrontFace, glFrontFace, PFNGLFRONTFACEPROC);
    GET_PROC(Hint, glHint, PFNGLHINTPROC);
    GET_PROC(LineWidth, glLineWidth, PFNGLLINEWIDTHPROC);
    GET_PROC(PointSize, glPointSize, PFNGLPOINTSIZEPROC);
    GET_PROC(PolygonMode, glPolygonMode, PFNGLPOLYGONMODEPROC);
    GET_PROC(Scissor, glScissor, PFNGLSCISSORPROC);
    GET_PROC(TexParameterf, glTexParameterf, PFNGLTEXPARAMETERFPROC);
    GET_PROC(TexParameterfv, glTexParameterfv, PFNGLTEXPARAMETERFVPROC);
    GET_PROC(TexParameteri, glTexParameteri, PFNGLTEXPARAMETERIPROC);
    GET_PROC(TexParameteriv, glTexParameteriv, PFNGLTEXPARAMETERIVPROC);
    GET_PROC(TexImage1D, glTexImage1D, PFNGLTEXIMAGE1DPROC);
    GET_PROC(TexImage2D, glTexImage2D, PFNGLTEXIMAGE2DPROC);
    GET_PROC(DrawBuffer, glDrawBuffer, PFNGLDRAWBUFFERPROC);
    GET_PROC(Clear, glClear, PFNGLCLEARPROC);
    GET_PROC(ClearColor, glClearColor, PFNGLCLEARCOLORPROC);
    GET_PROC(ClearStencil, glClearStencil, PFNGLCLEARSTENCILPROC);
    GET_PROC(ClearDepth, glClearDepth, PFNGLCLEARDEPTHPROC);
    GET_PROC(StencilMask, glStencilMask, PFNGLSTENCILMASKPROC);
    GET_PROC(ColorMask, glColorMask, PFNGLCOLORMASKPROC);
    GET_PROC(DepthMask, glDepthMask, PFNGLDEPTHMASKPROC);
    GET_PROC(Disable, glDisable, PFNGLDISABLEPROC);
    GET_PROC(Enable, glEnable, PFNGLENABLEPROC);
    GET_PROC(Finish, glFinish, PFNGLFINISHPROC);
    GET_PROC(Flush, glFlush, PFNGLFLUSHPROC);
    GET_PROC(BlendFunc, glBlendFunc, PFNGLBLENDFUNCPROC);
    GET_PROC(LogicOp, glLogicOp, PFNGLLOGICOPPROC);
    GET_PROC(StencilFunc, glStencilFunc, PFNGLSTENCILFUNCPROC);
    GET_PROC(StencilOp, glStencilOp, PFNGLSTENCILOPPROC);
    GET_PROC(DepthFunc, glDepthFunc, PFNGLDEPTHFUNCPROC);
    GET_PROC(PixelStoref, glPixelStoref, PFNGLPIXELSTOREFPROC);
    GET_PROC(PixelStorei, glPixelStorei, PFNGLPIXELSTOREIPROC);
    GET_PROC(ReadBuffer, glReadBuffer, PFNGLREADBUFFERPROC);
    GET_PROC(ReadPixels, glReadPixels, PFNGLREADPIXELSPROC);
    GET_PROC(GetBooleanv, glGetBooleanv, PFNGLGETBOOLEANVPROC);
    GET_PROC(GetDoublev, glGetDoublev, PFNGLGETDOUBLEVPROC);
    GET_PROC(GetError, glGetError, PFNGLGETERRORPROC);
    GET_PROC(GetFloatv, glGetFloatv, PFNGLGETFLOATVPROC);
    GET_PROC(GetIntegerv, glGetIntegerv, PFNGLGETINTEGERVPROC);
    GET_PROC(GetString, glGetString, PFNGLGETSTRINGPROC);
    GET_PROC(GetTexImage, glGetTexImage, PFNGLGETTEXIMAGEPROC);
    GET_PROC(GetTexParameterfv, glGetTexParameterfv, PFNGLGETTEXPARAMETERFVPROC);
    GET_PROC(GetTexParameteriv, glGetTexParameteriv, PFNGLGETTEXPARAMETERIVPROC);
    GET_PROC(GetTexLevelParameterfv, glGetTexLevelParameterfv, PFNGLGETTEXLEVELPARAMETERFVPROC);
    GET_PROC(GetTexLevelParameteriv, glGetTexLevelParameteriv, PFNGLGETTEXLEVELPARAMETERIVPROC);
    GET_PROC(IsEnabled, glIsEnabled, PFNGLISENABLEDPROC);
    GET_PROC(DepthRange, glDepthRange, PFNGLDEPTHRANGEPROC);
    GET_PROC(Viewport, glViewport, PFNGLVIEWPORTPROC);

    // Load OpenGL 1.1 functions
    GET_PROC(DrawArrays, glDrawArrays, PFNGLDRAWARRAYSPROC);
    GET_PROC(DrawElements, glDrawElements, PFNGLDRAWELEMENTSPROC);
    GET_PROC(GetPointerv, glGetPointerv, PFNGLGETPOINTERVPROC);
    GET_PROC(PolygonOffset, glPolygonOffset, PFNGLPOLYGONOFFSETPROC);
    GET_PROC(CopyTexImage1D, glCopyTexImage1D, PFNGLCOPYTEXIMAGE1DPROC);
    GET_PROC(CopyTexImage2D, glCopyTexImage2D, PFNGLCOPYTEXIMAGE2DPROC);
    GET_PROC(CopyTexSubImage1D, glCopyTexSubImage1D, PFNGLCOPYTEXSUBIMAGE1DPROC);
    GET_PROC(CopyTexSubImage2D, glCopyTexSubImage2D, PFNGLCOPYTEXSUBIMAGE2DPROC);
    GET_PROC(TexSubImage1D, glTexSubImage1D, PFNGLTEXSUBIMAGE1DPROC);
    GET_PROC(TexSubImage2D, glTexSubImage2D, PFNGLTEXSUBIMAGE2DPROC);
    GET_PROC(BindTexture, glBindTexture, PFNGLBINDTEXTUREPROC);
    GET_PROC(DeleteTextures, glDeleteTextures, PFNGLDELETETEXTURESPROC);
    GET_PROC(GenTextures, glGenTextures, PFNGLGENTEXTURESPROC);
    GET_PROC(IsTexture, glIsTexture, PFNGLISTEXTUREPROC);

#if defined(__WIN32__)
  // Now, we load everything with glut on Windows too
  #undef GET_PROC
  #define GET_PROC(FIELD,NAME,PROTOTYPE) do {                 \
    this->FIELD = (PROTOTYPE) glutGetProcAddress(#NAME);      \
    FATAL_IF (this->FIELD == NULL, "OpenGL 3.3 is required"); \
  } while (0)
#endif /* __WIN32__ */

    // OpenGL 1.2 functions
    GET_PROC(BlendColor, glBlendColor, PFNGLBLENDCOLORPROC);
    GET_PROC(BlendEquation, glBlendEquation, PFNGLBLENDEQUATIONPROC);
    GET_PROC(DrawRangeElements, glDrawRangeElements, PFNGLDRAWRANGEELEMENTSPROC);
    GET_PROC(TexImage3D, glTexImage3D, PFNGLTEXIMAGE3DPROC);
    GET_PROC(TexSubImage3D, glTexSubImage3D, PFNGLTEXSUBIMAGE3DPROC);
    GET_PROC(CopyTexSubImage3D, glCopyTexSubImage3D, PFNGLCOPYTEXSUBIMAGE3DPROC);

    // Load OpenGL 1.3 functions
    GET_PROC(ActiveTexture, glActiveTexture, PFNGLACTIVETEXTUREPROC);
    GET_PROC(SampleCoverage, glSampleCoverage, PFNGLSAMPLECOVERAGEPROC);
    GET_PROC(CompressedTexImage3D, glCompressedTexImage3D, PFNGLCOMPRESSEDTEXIMAGE3DPROC);
    GET_PROC(CompressedTexImage2D, glCompressedTexImage2D, PFNGLCOMPRESSEDTEXIMAGE2DPROC);
    GET_PROC(CompressedTexImage1D, glCompressedTexImage1D, PFNGLCOMPRESSEDTEXIMAGE1DPROC);
    GET_PROC(CompressedTexSubImage3D, glCompressedTexSubImage3D, PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC);
    GET_PROC(CompressedTexSubImage2D, glCompressedTexSubImage2D, PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC);
    GET_PROC(CompressedTexSubImage1D, glCompressedTexSubImage1D, PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC);
    GET_PROC(GetCompressedTexImage, glGetCompressedTexImage, PFNGLGETCOMPRESSEDTEXIMAGEPROC);

    // Load OpenGL 1.5 functions
    GET_PROC(GenQueries, glGenQueries, PFNGLGENQUERIESPROC);
    GET_PROC(DeleteQueries, glDeleteQueries, PFNGLDELETEQUERIESPROC);
    GET_PROC(IsQuery, glIsQuery, PFNGLISQUERYPROC);
    GET_PROC(BeginQuery, glBeginQuery, PFNGLBEGINQUERYPROC);
    GET_PROC(EndQuery, glEndQuery, PFNGLENDQUERYPROC);
    GET_PROC(GetQueryiv, glGetQueryiv, PFNGLGETQUERYIVPROC);
    GET_PROC(GetQueryObjectiv, glGetQueryObjectiv, PFNGLGETQUERYOBJECTIVPROC);
    GET_PROC(GetQueryObjectuiv, glGetQueryObjectuiv, PFNGLGETQUERYOBJECTUIVPROC);
    GET_PROC(BindBuffer, glBindBuffer, PFNGLBINDBUFFERPROC);
    GET_PROC(DeleteBuffers, glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    GET_PROC(GenBuffers, glGenBuffers, PFNGLGENBUFFERSPROC);
    GET_PROC(IsBuffer, glIsBuffer, PFNGLISBUFFERPROC);
    GET_PROC(BufferData, glBufferData, PFNGLBUFFERDATAPROC);
    GET_PROC(BufferSubData, glBufferSubData, PFNGLBUFFERSUBDATAPROC);
    GET_PROC(GetBufferSubData, glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);
    GET_PROC(MapBuffer, glMapBuffer, PFNGLMAPBUFFERPROC);
    GET_PROC(UnmapBuffer, glUnmapBuffer, PFNGLUNMAPBUFFERPROC);
    GET_PROC(GetBufferParameteriv, glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC);
    GET_PROC(GetBufferPointerv, glGetBufferPointerv, PFNGLGETBUFFERPOINTERVPROC);

    // Load OpenGL 2.0 functions
    GET_PROC(BlendEquationSeparate, glBlendEquationSeparate, PFNGLBLENDEQUATIONSEPARATEPROC);
    GET_PROC(DrawBuffers, glDrawBuffers, PFNGLDRAWBUFFERSPROC);
    GET_PROC(StencilOpSeparate, glStencilOpSeparate, PFNGLSTENCILOPSEPARATEPROC);
    GET_PROC(StencilFuncSeparate, glStencilFuncSeparate, PFNGLSTENCILFUNCSEPARATEPROC);
    GET_PROC(StencilMaskSeparate, glStencilMaskSeparate, PFNGLSTENCILMASKSEPARATEPROC);
    GET_PROC(AttachShader, glAttachShader, PFNGLATTACHSHADERPROC);
    GET_PROC(BindAttribLocation, glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC);
    GET_PROC(CompileShader, glCompileShader, PFNGLCOMPILESHADERPROC);
    GET_PROC(CreateProgram, glCreateProgram, PFNGLCREATEPROGRAMPROC);
    GET_PROC(CreateShader, glCreateShader, PFNGLCREATESHADERPROC);
    GET_PROC(DeleteProgram, glDeleteProgram, PFNGLDELETEPROGRAMPROC);
    GET_PROC(DeleteShader, glDeleteShader, PFNGLDELETESHADERPROC);
    GET_PROC(DetachShader, glDetachShader, PFNGLDETACHSHADERPROC);
    GET_PROC(DisableVertexAttribArray, glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
    GET_PROC(EnableVertexAttribArray, glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    GET_PROC(GetActiveAttrib, glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC);
    GET_PROC(GetActiveUniform, glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC);
    GET_PROC(GetAttachedShaders, glGetAttachedShaders, PFNGLGETATTACHEDSHADERSPROC);
    GET_PROC(GetAttribLocation, glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC);
    GET_PROC(GetProgramiv, glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    GET_PROC(GetProgramInfoLog, glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    GET_PROC(GetShaderiv, glGetShaderiv, PFNGLGETSHADERIVPROC);
    GET_PROC(GetShaderInfoLog, glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    GET_PROC(GetShaderSource, glGetShaderSource, PFNGLGETSHADERSOURCEPROC);
    GET_PROC(GetUniformLocation, glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    GET_PROC(GetUniformfv, glGetUniformfv, PFNGLGETUNIFORMFVPROC);
    GET_PROC(GetUniformiv, glGetUniformiv, PFNGLGETUNIFORMIVPROC);
    GET_PROC(GetVertexAttribdv, glGetVertexAttribdv, PFNGLGETVERTEXATTRIBDVPROC);
    GET_PROC(GetVertexAttribfv, glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC);
    GET_PROC(GetVertexAttribiv, glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC);
    GET_PROC(GetVertexAttribPointerv, glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC);
    GET_PROC(IsProgram, glIsProgram, PFNGLISPROGRAMPROC);
    GET_PROC(IsShader, glIsShader, PFNGLISSHADERPROC);
    GET_PROC(LinkProgram, glLinkProgram, PFNGLLINKPROGRAMPROC);
    GET_PROC(ShaderSource, glShaderSource, PFNGLSHADERSOURCEPROC);
    GET_PROC(UseProgram, glUseProgram, PFNGLUSEPROGRAMPROC);
    GET_PROC(Uniform1f, glUniform1f, PFNGLUNIFORM1FPROC);
    GET_PROC(Uniform2f, glUniform2f, PFNGLUNIFORM2FPROC);
    GET_PROC(Uniform3f, glUniform3f, PFNGLUNIFORM3FPROC);
    GET_PROC(Uniform4f, glUniform4f, PFNGLUNIFORM4FPROC);
    GET_PROC(Uniform1i, glUniform1i, PFNGLUNIFORM1IPROC);
    GET_PROC(Uniform2i, glUniform2i, PFNGLUNIFORM2IPROC);
    GET_PROC(Uniform3i, glUniform3i, PFNGLUNIFORM3IPROC);
    GET_PROC(Uniform4i, glUniform4i, PFNGLUNIFORM4IPROC);
    GET_PROC(Uniform1fv, glUniform1fv, PFNGLUNIFORM1FVPROC);
    GET_PROC(Uniform2fv, glUniform2fv, PFNGLUNIFORM2FVPROC);
    GET_PROC(Uniform3fv, glUniform3fv, PFNGLUNIFORM3FVPROC);
    GET_PROC(Uniform4fv, glUniform4fv, PFNGLUNIFORM4FVPROC);
    GET_PROC(Uniform1iv, glUniform1iv, PFNGLUNIFORM1IVPROC);
    GET_PROC(Uniform2iv, glUniform2iv, PFNGLUNIFORM2IVPROC);
    GET_PROC(Uniform3iv, glUniform3iv, PFNGLUNIFORM3IVPROC);
    GET_PROC(Uniform4iv, glUniform4iv, PFNGLUNIFORM4IVPROC);
    GET_PROC(UniformMatrix2fv, glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC);
    GET_PROC(UniformMatrix3fv, glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC);
    GET_PROC(UniformMatrix4fv, glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
    GET_PROC(ValidateProgram, glValidateProgram, PFNGLVALIDATEPROGRAMPROC);
    GET_PROC(VertexAttrib1d, glVertexAttrib1d, PFNGLVERTEXATTRIB1DPROC);
    GET_PROC(VertexAttrib1dv, glVertexAttrib1dv, PFNGLVERTEXATTRIB1DVPROC);
    GET_PROC(VertexAttrib1f, glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC);
    GET_PROC(VertexAttrib1fv, glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC);
    GET_PROC(VertexAttrib1s, glVertexAttrib1s, PFNGLVERTEXATTRIB1SPROC);
    GET_PROC(VertexAttrib1sv, glVertexAttrib1sv, PFNGLVERTEXATTRIB1SVPROC);
    GET_PROC(VertexAttrib2d, glVertexAttrib2d, PFNGLVERTEXATTRIB2DPROC);
    GET_PROC(VertexAttrib2dv, glVertexAttrib2dv, PFNGLVERTEXATTRIB2DVPROC);
    GET_PROC(VertexAttrib2f, glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC);
    GET_PROC(VertexAttrib2fv, glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC);
    GET_PROC(VertexAttrib2s, glVertexAttrib2s, PFNGLVERTEXATTRIB2SPROC);
    GET_PROC(VertexAttrib2sv, glVertexAttrib2sv, PFNGLVERTEXATTRIB2SVPROC);
    GET_PROC(VertexAttrib3d, glVertexAttrib3d, PFNGLVERTEXATTRIB3DPROC);
    GET_PROC(VertexAttrib3dv, glVertexAttrib3dv, PFNGLVERTEXATTRIB3DVPROC);
    GET_PROC(VertexAttrib3f, glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC);
    GET_PROC(VertexAttrib3fv, glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC);
    GET_PROC(VertexAttrib3s, glVertexAttrib3s, PFNGLVERTEXATTRIB3SPROC);
    GET_PROC(VertexAttrib3sv, glVertexAttrib3sv, PFNGLVERTEXATTRIB3SVPROC);
    GET_PROC(VertexAttrib4Nbv, glVertexAttrib4Nbv, PFNGLVERTEXATTRIB4NBVPROC);
    GET_PROC(VertexAttrib4Niv, glVertexAttrib4Niv, PFNGLVERTEXATTRIB4NIVPROC);
    GET_PROC(VertexAttrib4Nsv, glVertexAttrib4Nsv, PFNGLVERTEXATTRIB4NSVPROC);
    GET_PROC(VertexAttrib4Nub, glVertexAttrib4Nub, PFNGLVERTEXATTRIB4NUBPROC);
    GET_PROC(VertexAttrib4Nubv, glVertexAttrib4Nubv, PFNGLVERTEXATTRIB4NUBVPROC);
    GET_PROC(VertexAttrib4Nuiv, glVertexAttrib4Nuiv, PFNGLVERTEXATTRIB4NUIVPROC);
    GET_PROC(VertexAttrib4Nusv, glVertexAttrib4Nusv, PFNGLVERTEXATTRIB4NUSVPROC);
    GET_PROC(VertexAttrib4bv, glVertexAttrib4bv, PFNGLVERTEXATTRIB4BVPROC);
    GET_PROC(VertexAttrib4d, glVertexAttrib4d, PFNGLVERTEXATTRIB4DPROC);
    GET_PROC(VertexAttrib4dv, glVertexAttrib4dv, PFNGLVERTEXATTRIB4DVPROC);
    GET_PROC(VertexAttrib4f, glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC);
    GET_PROC(VertexAttrib4fv, glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC);
    GET_PROC(VertexAttrib4iv, glVertexAttrib4iv, PFNGLVERTEXATTRIB4IVPROC);
    GET_PROC(VertexAttrib4s, glVertexAttrib4s, PFNGLVERTEXATTRIB4SPROC);
    GET_PROC(VertexAttrib4sv, glVertexAttrib4sv, PFNGLVERTEXATTRIB4SVPROC);
    GET_PROC(VertexAttrib4ubv, glVertexAttrib4ubv, PFNGLVERTEXATTRIB4UBVPROC);
    GET_PROC(VertexAttrib4uiv, glVertexAttrib4uiv, PFNGLVERTEXATTRIB4UIVPROC);
    GET_PROC(VertexAttrib4usv, glVertexAttrib4usv, PFNGLVERTEXATTRIB4USVPROC);
    GET_PROC(VertexAttribPointer, glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);

    // Load OpenGL 3.0 functions
    GET_PROC(BindBufferBase, glBindBufferBase, PFNGLBINDBUFFERBASEPROC);
    GET_PROC(BindFragDataLocation, glBindFragDataLocation, PFNGLBINDFRAGDATALOCATIONPROC);
    GET_PROC(GenerateMipmap, glGenerateMipmap, PFNGLGENERATEMIPMAPPROC);
    GET_PROC(DeleteVertexArrays, glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC);
    GET_PROC(GenVertexArrays, glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    GET_PROC(BindVertexArray, glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    GET_PROC(GenFramebuffers, glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC);
    GET_PROC(BindFramebuffer, glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
    GET_PROC(FramebufferTextureLayer, glFramebufferTextureLayer, PFNGLFRAMEBUFFERTEXTURELAYERPROC);
    GET_PROC(FramebufferTexture2D, glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC);
    GET_PROC(CheckFramebufferStatus, glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC);
    GET_PROC(DeleteFramebuffers, glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC);
    GET_PROC(MapBufferRange, glMapBufferRange, PFNGLMAPBUFFERRANGEPROC);
    GET_PROC(FlushMappedBufferRange, glFlushMappedBufferRange, PFNGLFLUSHMAPPEDBUFFERRANGEPROC);
    GET_PROC(GenRenderbuffers, glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC);
    GET_PROC(BindRenderbuffer, glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC);
    GET_PROC(RenderbufferStorage, glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC);
    GET_PROC(FramebufferRenderbuffer, glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC);
    GET_PROC(BlitFramebuffer, glBlitFramebuffer, PFNGLBLITFRAMEBUFFERPROC);
    GET_PROC(DeleteRenderbuffers, glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC);
    GET_PROC(RenderbufferStorageMultisample, glRenderbufferStorageMultisample, PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC);
    GET_PROC(ColorMaski, glColorMaski, PFNGLCOLORMASKIPROC);
    GET_PROC(GetBooleani_v, glGetBooleani_v, PFNGLGETBOOLEANI_VPROC);
    GET_PROC(GetIntegeri_v, glGetIntegeri_v, PFNGLGETINTEGERI_VPROC);
    GET_PROC(Enablei, glEnablei, PFNGLENABLEIPROC);
    GET_PROC(Disablei, glDisablei, PFNGLDISABLEIPROC);

    // Load OpenGL 3.1 functions
    GET_PROC(BindBufferBase, glBindBufferBase, PFNGLBINDBUFFERBASEPROC);
    GET_PROC(BindBufferRange, glBindBufferRange, PFNGLBINDBUFFERRANGEPROC);
    GET_PROC(DrawArraysInstanced, glDrawArraysInstanced, PFNGLDRAWARRAYSINSTANCEDPROC);
    GET_PROC(DrawElementsInstanced, glDrawElementsInstanced, PFNGLDRAWELEMENTSINSTANCEDPROC);
    GET_PROC(TexBuffer, glTexBuffer, PFNGLTEXBUFFERPROC);
    GET_PROC(PrimitiveRestartIndex, glPrimitiveRestartIndex, PFNGLPRIMITIVERESTARTINDEXPROC);
    GET_PROC(GetUniformIndices, glGetUniformIndices, PFNGLGETUNIFORMINDICESPROC);
    GET_PROC(GetActiveUniformsiv, glGetActiveUniformsiv, PFNGLGETACTIVEUNIFORMSIVPROC);
    GET_PROC(GetActiveUniformName, glGetActiveUniformName, PFNGLGETACTIVEUNIFORMNAMEPROC);
    GET_PROC(GetUniformBlockIndex, glGetUniformBlockIndex, PFNGLGETUNIFORMBLOCKINDEXPROC);
    GET_PROC(GetActiveUniformBlockiv, glGetActiveUniformBlockiv, PFNGLGETACTIVEUNIFORMBLOCKIVPROC);
    GET_PROC(GetActiveUniformBlockName, glGetActiveUniformBlockName, PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC);
    GET_PROC(UniformBlockBinding, glUniformBlockBinding, PFNGLUNIFORMBLOCKBINDINGPROC);

    // Load OpenGL 3.2 functions
    GET_PROC(GetInteger64i_v, glGetInteger64i_v, PFNGLGETINTEGER64I_VPROC);
    GET_PROC(GetBufferParameteri64v, glGetBufferParameteri64v, PFNGLGETBUFFERPARAMETERI64VPROC);
    GET_PROC(FramebufferTexture, glFramebufferTexture, PFNGLFRAMEBUFFERTEXTUREPROC);
#undef GET_PROC

    // Get driver dependent constants
#define GET_CST(ENUM, FIELD)                        \
    this->GetIntegerv(ENUM, &this->FIELD);          \
    fprintf(stdout, #ENUM " == %i\n", this->FIELD);
    GET_CST(GL_MAX_COLOR_ATTACHMENTS, maxColorAttachmentNum);
    GET_CST(GL_MAX_TEXTURE_SIZE, maxTextureSize);
    GET_CST(GL_MAX_TEXTURE_IMAGE_UNITS, maxTextureUnit);
#undef GET_CST
  }

  OGL::~OGL() {}

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


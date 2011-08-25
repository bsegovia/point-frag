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
    // Load OpenGL 1.0 functions
    this->CullFace = (PFNGLCULLFACEPROC)glutGetProcAddress("glCullFace");
    this->FrontFace = (PFNGLFRONTFACEPROC)glutGetProcAddress("glFrontFace");
    this->Hint = (PFNGLHINTPROC)glutGetProcAddress("glHint");
    this->LineWidth = (PFNGLLINEWIDTHPROC)glutGetProcAddress("glLineWidth");
    this->PointSize = (PFNGLPOINTSIZEPROC)glutGetProcAddress("glPointSize");
    this->PolygonMode = (PFNGLPOLYGONMODEPROC)glutGetProcAddress("glPolygonMode");
    this->Scissor = (PFNGLSCISSORPROC)glutGetProcAddress("glScissor");
    this->TexParameterf = (PFNGLTEXPARAMETERFPROC)glutGetProcAddress("glTexParameterf");
    this->TexParameterfv = (PFNGLTEXPARAMETERFVPROC)glutGetProcAddress("glTexParameterfv");
    this->TexParameteri = (PFNGLTEXPARAMETERIPROC)glutGetProcAddress("glTexParameteri");
    this->TexParameteriv = (PFNGLTEXPARAMETERIVPROC)glutGetProcAddress("glTexParameteriv");
    this->TexImage1D = (PFNGLTEXIMAGE1DPROC)glutGetProcAddress("glTexImage1D");
    this->TexImage2D = (PFNGLTEXIMAGE2DPROC)glutGetProcAddress("glTexImage2D");
    this->DrawBuffer = (PFNGLDRAWBUFFERPROC)glutGetProcAddress("glDrawBuffer");
    this->Clear = (PFNGLCLEARPROC)glutGetProcAddress("glClear");
    this->ClearColor = (PFNGLCLEARCOLORPROC)glutGetProcAddress("glClearColor");
    this->ClearStencil = (PFNGLCLEARSTENCILPROC)glutGetProcAddress("glClearStencil");
    this->ClearDepth = (PFNGLCLEARDEPTHPROC)glutGetProcAddress("glClearDepth");
    this->StencilMask = (PFNGLSTENCILMASKPROC)glutGetProcAddress("glStencilMask");
    this->ColorMask = (PFNGLCOLORMASKPROC)glutGetProcAddress("glColorMask");
    this->DepthMask = (PFNGLDEPTHMASKPROC)glutGetProcAddress("glDepthMask");
    this->Disable = (PFNGLDISABLEPROC)glutGetProcAddress("glDisable");
    this->Enable = (PFNGLENABLEPROC)glutGetProcAddress("glEnable");
    this->Finish = (PFNGLFINISHPROC)glutGetProcAddress("glFinish");
    this->Flush = (PFNGLFLUSHPROC)glutGetProcAddress("glFlush");
    this->BlendFunc = (PFNGLBLENDFUNCPROC)glutGetProcAddress("glBlendFunc");
    this->LogicOp = (PFNGLLOGICOPPROC)glutGetProcAddress("glLogicOp");
    this->StencilFunc = (PFNGLSTENCILFUNCPROC)glutGetProcAddress("glStencilFunc");
    this->StencilOp = (PFNGLSTENCILOPPROC)glutGetProcAddress("glStencilOp");
    this->DepthFunc = (PFNGLDEPTHFUNCPROC)glutGetProcAddress("glDepthFunc");
    this->PixelStoref = (PFNGLPIXELSTOREFPROC)glutGetProcAddress("glPixelStoref");
    this->PixelStorei = (PFNGLPIXELSTOREIPROC)glutGetProcAddress("glPixelStorei");
    this->ReadBuffer = (PFNGLREADBUFFERPROC)glutGetProcAddress("glReadBuffer");
    this->ReadPixels = (PFNGLREADPIXELSPROC)glutGetProcAddress("glReadPixels");
    this->GetBooleanv = (PFNGLGETBOOLEANVPROC)glutGetProcAddress("glGetBooleanv");
    this->GetDoublev = (PFNGLGETDOUBLEVPROC)glutGetProcAddress("glGetDoublev");
    this->GetError = (PFNGLGETERRORPROC)glutGetProcAddress("glGetError");
    this->GetFloatv = (PFNGLGETFLOATVPROC)glutGetProcAddress("glGetFloatv");
    this->GetIntegerv = (PFNGLGETINTEGERVPROC)glutGetProcAddress("glGetIntegerv");
    this->GetString = (PFNGLGETSTRINGPROC)glutGetProcAddress("glGetString");
    this->GetTexImage = (PFNGLGETTEXIMAGEPROC)glutGetProcAddress("glGetTexImage");
    this->GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)glutGetProcAddress("glGetTexParameterfv");
    this->GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)glutGetProcAddress("glGetTexParameteriv");
    this->GetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)glutGetProcAddress("glGetTexLevelParameterfv");
    this->GetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)glutGetProcAddress("glGetTexLevelParameteriv");
    this->IsEnabled = (PFNGLISENABLEDPROC)glutGetProcAddress("glIsEnabled");
    this->DepthRange = (PFNGLDEPTHRANGEPROC)glutGetProcAddress("glDepthRange");
    this->Viewport = (PFNGLVIEWPORTPROC)glutGetProcAddress("glViewport");

    // Load OpenGL 1.1 functions
    this->DrawArrays = (PFNGLDRAWARRAYSPROC)glutGetProcAddress("glDrawArrays");
    this->DrawElements = (PFNGLDRAWELEMENTSPROC)glutGetProcAddress("glDrawElements");
    this->GetPointerv = (PFNGLGETPOINTERVPROC)glutGetProcAddress("glGetPointerv");
    this->PolygonOffset = (PFNGLPOLYGONOFFSETPROC)glutGetProcAddress("glPolygonOffset");
    this->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)glutGetProcAddress("glCopyTexImage1D");
    this->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)glutGetProcAddress("glCopyTexImage2D");
    this->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)glutGetProcAddress("glCopyTexSubImage1D");
    this->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)glutGetProcAddress("glCopyTexSubImage2D");
    this->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)glutGetProcAddress("glTexSubImage1D");
    this->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)glutGetProcAddress("glTexSubImage2D");
    this->BindTexture = (PFNGLBINDTEXTUREPROC)glutGetProcAddress("glBindTexture");
    this->DeleteTextures = (PFNGLDELETETEXTURESPROC)glutGetProcAddress("glDeleteTextures");
    this->GenTextures = (PFNGLGENTEXTURESPROC)glutGetProcAddress("glGenTextures");
    this->IsTexture = (PFNGLISTEXTUREPROC)glutGetProcAddress("glIsTexture");

    // Load OpenGL 1.3 functions
    this->ActiveTexture = (PFNGLACTIVETEXTUREPROC)glutGetProcAddress("glActiveTexture");
    this->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)glutGetProcAddress("glSampleCoverage");
    this->CompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)glutGetProcAddress("glCompressedTexImage3D");
    this->CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)glutGetProcAddress("glCompressedTexImage2D");
    this->CompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)glutGetProcAddress("glCompressedTexImage1D");
    this->CompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)glutGetProcAddress("glCompressedTexSubImage3D");
    this->CompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)glutGetProcAddress("glCompressedTexSubImage2D");
    this->CompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)glutGetProcAddress("glCompressedTexSubImage1D");
    this->GetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)glutGetProcAddress("glGetCompressedTexImage");

    // OpenGL 1.2 functions
    this->BlendColor = (PFNGLBLENDCOLORPROC)glutGetProcAddress("glBlendColor");
    this->BlendEquation = (PFNGLBLENDEQUATIONPROC)glutGetProcAddress("glBlendEquation");
    this->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)glutGetProcAddress("glDrawRangeElements");
    this->TexImage3D = (PFNGLTEXIMAGE3DPROC)glutGetProcAddress("glTexImage3D");
    this->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)glutGetProcAddress("glTexSubImage3D");
    this->CopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)glutGetProcAddress("glCopyTexSubImage3D");

    // Load OpenGL 1.5 functions
    this->GenQueries = (PFNGLGENQUERIESPROC)glutGetProcAddress("glGenQueries");
    this->DeleteQueries = (PFNGLDELETEQUERIESPROC)glutGetProcAddress("glDeleteQueries");
    this->IsQuery = (PFNGLISQUERYPROC)glutGetProcAddress("glIsQuery");
    this->BeginQuery = (PFNGLBEGINQUERYPROC)glutGetProcAddress("glBeginQuery");
    this->EndQuery = (PFNGLENDQUERYPROC)glutGetProcAddress("glEndQuery");
    this->GetQueryiv = (PFNGLGETQUERYIVPROC)glutGetProcAddress("glGetQueryiv");
    this->GetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)glutGetProcAddress("glGetQueryObjectiv");
    this->GetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)glutGetProcAddress("glGetQueryObjectuiv");
    this->BindBuffer = (PFNGLBINDBUFFERPROC)glutGetProcAddress("glBindBuffer");
    this->DeleteBuffers = (PFNGLDELETEBUFFERSPROC)glutGetProcAddress("glDeleteBuffers");
    this->GenBuffers = (PFNGLGENBUFFERSPROC)glutGetProcAddress("glGenBuffers");
    this->IsBuffer = (PFNGLISBUFFERPROC)glutGetProcAddress("glIsBuffer");
    this->BufferData = (PFNGLBUFFERDATAPROC)glutGetProcAddress("glBufferData");
    this->BufferSubData = (PFNGLBUFFERSUBDATAPROC)glutGetProcAddress("glBufferSubData");
    this->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)glutGetProcAddress("glGetBufferSubData");
    this->MapBuffer = (PFNGLMAPBUFFERPROC)glutGetProcAddress("glMapBuffer");
    this->UnmapBuffer = (PFNGLUNMAPBUFFERPROC)glutGetProcAddress("glUnmapBuffer");
    this->GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)glutGetProcAddress("glGetBufferParameteriv");
    this->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)glutGetProcAddress("glGetBufferPointerv");

    // Load OpenGL 2.0 functions
    this->BlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)glutGetProcAddress("glBlendEquationSeparate");
    this->DrawBuffers = (PFNGLDRAWBUFFERSPROC)glutGetProcAddress("glDrawBuffers");
    this->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)glutGetProcAddress("glStencilOpSeparate");
    this->StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)glutGetProcAddress("glStencilFuncSeparate");
    this->StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)glutGetProcAddress("glStencilMaskSeparate");
    this->AttachShader = (PFNGLATTACHSHADERPROC)glutGetProcAddress("glAttachShader");
    this->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)glutGetProcAddress("glBindAttribLocation");
    this->CompileShader = (PFNGLCOMPILESHADERPROC)glutGetProcAddress("glCompileShader");
    this->CreateProgram = (PFNGLCREATEPROGRAMPROC)glutGetProcAddress("glCreateProgram");
    this->CreateShader = (PFNGLCREATESHADERPROC)glutGetProcAddress("glCreateShader");
    this->DeleteProgram = (PFNGLDELETEPROGRAMPROC)glutGetProcAddress("glDeleteProgram");
    this->DeleteShader = (PFNGLDELETESHADERPROC)glutGetProcAddress("glDeleteShader");
    this->DetachShader = (PFNGLDETACHSHADERPROC)glutGetProcAddress("glDetachShader");
    this->DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)glutGetProcAddress("glDisableVertexAttribArray");
    this->EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glutGetProcAddress("glEnableVertexAttribArray");
    this->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)glutGetProcAddress("glGetActiveAttrib");
    this->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)glutGetProcAddress("glGetActiveUniform");
    this->GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)glutGetProcAddress("glGetAttachedShaders");
    this->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)glutGetProcAddress("glGetAttribLocation");
    this->GetProgramiv = (PFNGLGETPROGRAMIVPROC)glutGetProcAddress("glGetProgramiv");
    this->GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glutGetProcAddress("glGetProgramInfoLog");
    this->GetShaderiv = (PFNGLGETSHADERIVPROC)glutGetProcAddress("glGetShaderiv");
    this->GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glutGetProcAddress("glGetShaderInfoLog");
    this->GetShaderSource = (PFNGLGETSHADERSOURCEPROC)glutGetProcAddress("glGetShaderSource");
    this->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glutGetProcAddress("glGetUniformLocation");
    this->GetUniformfv = (PFNGLGETUNIFORMFVPROC)glutGetProcAddress("glGetUniformfv");
    this->GetUniformiv = (PFNGLGETUNIFORMIVPROC)glutGetProcAddress("glGetUniformiv");
    this->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)glutGetProcAddress("glGetVertexAttribdv");
    this->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)glutGetProcAddress("glGetVertexAttribfv");
    this->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)glutGetProcAddress("glGetVertexAttribiv");
    this->GetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)glutGetProcAddress("glGetVertexAttribPointerv");
    this->IsProgram = (PFNGLISPROGRAMPROC)glutGetProcAddress("glIsProgram");
    this->IsShader = (PFNGLISSHADERPROC)glutGetProcAddress("glIsShader");
    this->LinkProgram = (PFNGLLINKPROGRAMPROC)glutGetProcAddress("glLinkProgram");
    this->ShaderSource = (PFNGLSHADERSOURCEPROC)glutGetProcAddress("glShaderSource");
    this->UseProgram = (PFNGLUSEPROGRAMPROC)glutGetProcAddress("glUseProgram");
    this->Uniform1f = (PFNGLUNIFORM1FPROC)glutGetProcAddress("glUniform1f");
    this->Uniform2f = (PFNGLUNIFORM2FPROC)glutGetProcAddress("glUniform2f");
    this->Uniform3f = (PFNGLUNIFORM3FPROC)glutGetProcAddress("glUniform3f");
    this->Uniform4f = (PFNGLUNIFORM4FPROC)glutGetProcAddress("glUniform4f");
    this->Uniform1i = (PFNGLUNIFORM1IPROC)glutGetProcAddress("glUniform1i");
    this->Uniform2i = (PFNGLUNIFORM2IPROC)glutGetProcAddress("glUniform2i");
    this->Uniform3i = (PFNGLUNIFORM3IPROC)glutGetProcAddress("glUniform3i");
    this->Uniform4i = (PFNGLUNIFORM4IPROC)glutGetProcAddress("glUniform4i");
    this->Uniform1fv = (PFNGLUNIFORM1FVPROC)glutGetProcAddress("glUniform1fv");
    this->Uniform2fv = (PFNGLUNIFORM2FVPROC)glutGetProcAddress("glUniform2fv");
    this->Uniform3fv = (PFNGLUNIFORM3FVPROC)glutGetProcAddress("glUniform3fv");
    this->Uniform4fv = (PFNGLUNIFORM4FVPROC)glutGetProcAddress("glUniform4fv");
    this->Uniform1iv = (PFNGLUNIFORM1IVPROC)glutGetProcAddress("glUniform1iv");
    this->Uniform2iv = (PFNGLUNIFORM2IVPROC)glutGetProcAddress("glUniform2iv");
    this->Uniform3iv = (PFNGLUNIFORM3IVPROC)glutGetProcAddress("glUniform3iv");
    this->Uniform4iv = (PFNGLUNIFORM4IVPROC)glutGetProcAddress("glUniform4iv");
    this->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)glutGetProcAddress("glUniformMatrix2fv");
    this->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)glutGetProcAddress("glUniformMatrix3fv");
    this->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)glutGetProcAddress("glUniformMatrix4fv");
    this->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)glutGetProcAddress("glValidateProgram");
    this->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)glutGetProcAddress("glVertexAttrib1d");
    this->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)glutGetProcAddress("glVertexAttrib1dv");
    this->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)glutGetProcAddress("glVertexAttrib1f");
    this->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)glutGetProcAddress("glVertexAttrib1fv");
    this->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)glutGetProcAddress("glVertexAttrib1s");
    this->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)glutGetProcAddress("glVertexAttrib1sv");
    this->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)glutGetProcAddress("glVertexAttrib2d");
    this->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)glutGetProcAddress("glVertexAttrib2dv");
    this->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)glutGetProcAddress("glVertexAttrib2f");
    this->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)glutGetProcAddress("glVertexAttrib2fv");
    this->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)glutGetProcAddress("glVertexAttrib2s");
    this->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)glutGetProcAddress("glVertexAttrib2sv");
    this->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)glutGetProcAddress("glVertexAttrib3d");
    this->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)glutGetProcAddress("glVertexAttrib3dv");
    this->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)glutGetProcAddress("glVertexAttrib3f");
    this->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)glutGetProcAddress("glVertexAttrib3fv");
    this->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)glutGetProcAddress("glVertexAttrib3s");
    this->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)glutGetProcAddress("glVertexAttrib3sv");
    this->VertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)glutGetProcAddress("glVertexAttrib4Nbv");
    this->VertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)glutGetProcAddress("glVertexAttrib4Niv");
    this->VertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)glutGetProcAddress("glVertexAttrib4Nsv");
    this->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)glutGetProcAddress("glVertexAttrib4Nub");
    this->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)glutGetProcAddress("glVertexAttrib4Nubv");
    this->VertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)glutGetProcAddress("glVertexAttrib4Nuiv");
    this->VertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)glutGetProcAddress("glVertexAttrib4Nusv");
    this->VertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)glutGetProcAddress("glVertexAttrib4bv");
    this->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)glutGetProcAddress("glVertexAttrib4d");
    this->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)glutGetProcAddress("glVertexAttrib4dv");
    this->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)glutGetProcAddress("glVertexAttrib4f");
    this->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)glutGetProcAddress("glVertexAttrib4fv");
    this->VertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)glutGetProcAddress("glVertexAttrib4iv");
    this->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)glutGetProcAddress("glVertexAttrib4s");
    this->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)glutGetProcAddress("glVertexAttrib4sv");
    this->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)glutGetProcAddress("glVertexAttrib4ubv");
    this->VertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)glutGetProcAddress("glVertexAttrib4uiv");
    this->VertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)glutGetProcAddress("glVertexAttrib4usv");
    this->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glutGetProcAddress("glVertexAttribPointer");

    // Load OpenGL 3.0 functions
    this->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)glutGetProcAddress("glBindBufferBase");
    this->BindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)glutGetProcAddress("glBindFragDataLocation");
    this->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)glutGetProcAddress("glGenerateMipmap");
    this->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glutGetProcAddress("glDeleteVertexArrays");
    this->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glutGetProcAddress("glGenVertexArrays");
    this->BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glutGetProcAddress("glBindVertexArray");
    this->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)glutGetProcAddress("glGenFramebuffers");
    this->BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)glutGetProcAddress("glBindFramebuffer");
    this->FramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)glutGetProcAddress("glFramebufferTextureLayer");
    this->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)glutGetProcAddress("glFramebufferTexture2D");
    this->CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glutGetProcAddress("glCheckFramebufferStatus");
    this->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)glutGetProcAddress("glDeleteFramebuffers");
    this->MapBufferRange = (PFNGLMAPBUFFERRANGEPROC)glutGetProcAddress("glMapBufferRange");
    this->FlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)glutGetProcAddress("glFlushMappedBufferRange");
    this->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)glutGetProcAddress("glGenRenderbuffers");
    this->BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)glutGetProcAddress("glBindRenderbuffer");
    this->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)glutGetProcAddress("glRenderbufferStorage");
    this->FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glutGetProcAddress("glFramebufferRenderbuffer");
    this->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)glutGetProcAddress("glBlitFramebuffer");
    this->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)glutGetProcAddress("glDeleteRenderbuffers");
    this->RenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)glutGetProcAddress("glRenderbufferStorageMultisample");
    this->ColorMaski = (PFNGLCOLORMASKIPROC)glutGetProcAddress("glColorMaski");
    this->GetBooleani_v = (PFNGLGETBOOLEANI_VPROC)glutGetProcAddress("glGetBooleani_v");
    this->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC)glutGetProcAddress("glGetIntegeri_v");
    this->Enablei = (PFNGLENABLEIPROC)glutGetProcAddress("glEnablei");
    this->Disablei = (PFNGLDISABLEIPROC)glutGetProcAddress("glDisablei");

    // Load OpenGL 3.1 functions
    this->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)glutGetProcAddress("glBindBufferBase");
    this->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)glutGetProcAddress("glBindBufferRange");
    this->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)glutGetProcAddress("glDrawArraysInstanced");
    this->DrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)glutGetProcAddress("glDrawElementsInstanced");
    this->TexBuffer = (PFNGLTEXBUFFERPROC)glutGetProcAddress("glTexBuffer");
    this->PrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)glutGetProcAddress("glPrimitiveRestartIndex");
    this->GetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)glutGetProcAddress("glGetUniformIndices");
    this->GetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)glutGetProcAddress("glGetActiveUniformsiv");
    this->GetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)glutGetProcAddress("glGetActiveUniformName");
    this->GetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)glutGetProcAddress("glGetUniformBlockIndex");
    this->GetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)glutGetProcAddress("glGetActiveUniformBlockiv");
    this->GetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)glutGetProcAddress("glGetActiveUniformBlockName");
    this->UniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)glutGetProcAddress("glUniformBlockBinding");

    // Load OpenGL 3.2 functions
    this->GetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)glutGetProcAddress("glGetInteger64i_v");
    this->GetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)glutGetProcAddress("glGetBufferParameteri64v");
    this->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)glutGetProcAddress("glFramebufferTexture");

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
      default: errString = "GL_UNKKNOWN";
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


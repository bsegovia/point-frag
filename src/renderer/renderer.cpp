#include "renderer.hpp"
#include <cstring>

namespace pf
{
#define OGL_NAME this
#if 0
  /*! Where you may find data files and shaders */
  static const char *dataPath[] = {
    "./share/",
    "../share/",
    "../../share/",
    "./",
    "../",
    "../../",
    "./data/",
    "../data/",
    "../../data/",
  };
#endif

  GLuint Renderer::makeGBufferTexture(int internal, int w, int h, int data, int type)
  {
    GLuint tex = 0;
    R_CALL (GenTextures, 1, &tex);
    R_CALL (BindTexture, GL_TEXTURE_2D, tex);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    R_CALL (TexImage2D, GL_TEXTURE_2D, 0, internal, w, h, 0, data, type, NULL);
    return tex;
  }

  GLuint Renderer::buildProgram(const std::string &prefix,
                                bool useVertex,
                                bool useGeometry,
                                bool useFragment)
  {
    GLuint program = 0, fragmentName = 0, vertexName = 0, geometryName = 0;
    R_CALLR (program, CreateProgram);

    if (useVertex) {
      const FileName path = FileName(prefix + ".vert");
      R_CALLR (vertexName, createShader, GL_VERTEX_SHADER, path);
      R_CALL (AttachShader, program, vertexName);
      R_CALL (DeleteShader, vertexName);
    }
    if (useFragment) {
      const FileName path = FileName(prefix + ".frag");
      R_CALLR (fragmentName, createShader, GL_FRAGMENT_SHADER, path);
      R_CALL (AttachShader, program, fragmentName);
      R_CALL (DeleteShader, fragmentName);
    }
    if (useGeometry) {
      const FileName path = FileName(prefix + ".geom");
      R_CALLR (geometryName, createShader, GL_GEOMETRY_SHADER, path);
      R_CALL (AttachShader, program, geometryName);
      R_CALL (DeleteShader, geometryName);
    }
    R_CALL (LinkProgram, program);
    return program;
  }

  GLuint Renderer::buildProgram(const char *vertSource,
                                const char *geomSource,
                                const char *fragSource)
  {
    GLuint program = 0, fragmentName = 0, vertexName = 0, geometryName = 0;
    R_CALLR (program, CreateProgram);

    if (vertSource) {
      R_CALLR (vertexName, createShader, GL_VERTEX_SHADER, std::string(vertSource));
      R_CALL (AttachShader, program, vertexName);
      R_CALL (DeleteShader, vertexName);
    }
    if (fragSource) {
      R_CALLR (fragmentName, createShader, GL_FRAGMENT_SHADER, std::string(fragSource));
      R_CALL (AttachShader, program, fragmentName);
      R_CALL (DeleteShader, fragmentName);
    }
    if (geomSource) {
      R_CALLR (geometryName, createShader, GL_GEOMETRY_SHADER, std::string(geomSource));
      R_CALL (AttachShader, program, geometryName);
      R_CALL (DeleteShader, geometryName);
    }
    R_CALL (LinkProgram, program);
    return program;
  }

  void Renderer::initGBuffer(int w, int h)
  {
    R_CALL (ActiveTexture, GL_TEXTURE0);
    this->gbuffer.diffuse_exp = makeGBufferTexture(GL_RGBA16F, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
    this->gbuffer.radiance = makeGBufferTexture(GL_RGBA16F, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
    this->gbuffer.pxpypz_nx = makeGBufferTexture(GL_RGBA16F, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
    this->gbuffer.vxvy_nynz = makeGBufferTexture(GL_RGBA16F, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
    this->gbuffer.zbuffer = makeGBufferTexture(GL_DEPTH24_STENCIL8, w, h, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);

    R_CALL (GenFramebuffers, 1, &this->gbuffer.fbo);
    R_CALL (BindFramebuffer, GL_FRAMEBUFFER, this->gbuffer.fbo);
    R_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->gbuffer.diffuse_exp, 0);
    R_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, this->gbuffer.radiance, 0);
    R_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, this->gbuffer.pxpypz_nx, 0);
    R_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, this->gbuffer.vxvy_nynz, 0);
    R_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, this->gbuffer.zbuffer, 0);
    R_CALL (BindFramebuffer, GL_FRAMEBUFFER, 0);
  }

  void Renderer::destroyGBuffer(void)
  {
    if (this->gbuffer.fbo) R_CALL (DeleteFramebuffers, 1, &this->gbuffer.fbo);
    if (this->gbuffer.diffuse_exp) R_CALL (DeleteTextures, 1, &this->gbuffer.diffuse_exp);
    if (this->gbuffer.radiance) R_CALL (DeleteTextures, 1, &this->gbuffer.radiance);
    if (this->gbuffer.pxpypz_nx) R_CALL (DeleteTextures, 1, &this->gbuffer.pxpypz_nx);
    if (this->gbuffer.vxvy_nynz) R_CALL (DeleteTextures, 1, &this->gbuffer.vxvy_nynz);
    if (this->gbuffer.zbuffer) R_CALL (DeleteTextures, 1, &this->gbuffer.zbuffer);
  }

  // Plain shader -> typically to display bounding boxex, wireframe geometries
  static const char plainVert[] = {
    "#version 330 core\n"
    "#define POSITION 0\n"
    "uniform mat4 MVP;\n"
    "uniform vec4 c;\n"
    "layout(location = POSITION) in vec3 p;\n"
    "out block {\n"
    "  vec4 c;\n"
    "} Out;\n"
    "void main() {\n"
    "  Out.c = c;\n"
    "  gl_Position = MVP * vec4(p.x, -p.y, p.z, 1.f);\n"
    "}\n"
  };
  static const char plainFrag[] = {
    "#version 330 core\n"
    "#define FRAG_COLOR 0\n"
    "uniform sampler2D Diffuse;\n"
    "in block {\n"
    "  vec4 c;\n"
    "} In;\n"
    "layout(location = FRAG_COLOR, index = 0) out vec4 c;\n"
    "void main() {\n"
    "  c = In.c;\n"
    "}\n"
  };
  void Renderer::initPlain(void) {
    this->plain.program = buildProgram(plainVert, NULL, plainFrag);
    R_CALL (BindAttribLocation, this->plain.program, ATTR_POSITION, "Position");
    R_CALLR (this->plain.uMVP, GetUniformLocation, this->plain.program, "MVP");
    R_CALLR (this->plain.uCol, GetUniformLocation, this->plain.program, "c");
  }
  void Renderer::destroyPlain(void) {
    if (this->plain.program) {
      R_CALL (DeleteProgram, this->plain.program);
      std::memset(&this->plain, 0, sizeof(this->plain));
    }
  }

  /*! Bounding box indices */
  static const int bboxIndex[36] = {
    0,3,1, 1,3,2, 4,5,7, 5,6,7,
    5,1,2, 5,2,6, 0,3,4, 3,4,7,
    2,3,7, 2,7,6, 1,4,5, 0,1,3
  };

  void Renderer::displayBBox(const BBox3f *bbox, int n, const vec4f *col)
  {
    vec3f pts[8]; // 8 points of the bounding box
    R_CALL (UseProgram, this->plain.program);
    R_CALL (UniformMatrix4fv, this->plain.uMVP, 1, GL_FALSE, &this->MVP[0][0]);
    R_CALL (Disable, GL_CULL_FACE);
    R_CALL (PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
    if (col == NULL)
      R_CALL (Uniform4fv, this->plain.uCol, 1, &this->defaultDiffuseCol.x);
    for (int i = 0; i < n; ++i) {
      if (col) R_CALL (Uniform4fv, this->plain.uCol, 4, &col[i].x);
      pts[0].x = pts[3].x = pts[4].x = pts[7].x = bbox[i].lower.x;
      pts[0].y = pts[1].y = pts[4].y = pts[5].y = bbox[i].lower.y;
      pts[0].z = pts[1].z = pts[2].z = pts[3].z = bbox[i].lower.z;
      pts[1].x = pts[2].x = pts[6].x = pts[5].x = bbox[i].upper.x;
      pts[2].y = pts[3].y = pts[6].y = pts[7].y = bbox[i].upper.y;
      pts[4].z = pts[5].z = pts[6].z = pts[7].z = bbox[i].upper.z;
      R_CALL (EnableVertexAttribArray, ATTR_POSITION);
      R_CALL (VertexAttribPointer, ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), &pts[0].x);
      R_CALL (DrawElements, GL_TRIANGLES, 36, GL_UNSIGNED_INT, bboxIndex);
    }
    R_CALL (DisableVertexAttribArray, ATTR_POSITION);
    R_CALL (Enable, GL_CULL_FACE);
    R_CALL (UseProgram, 0);
    R_CALL (PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
  }

  Renderer::Renderer(void) {
    std::memset(&this->gbuffer, 0, sizeof(this->gbuffer));
    this->initPlain();
    this->defaultDiffuseCol = this->defaultSpecularCol = vec4f(1.f,0.f,0.f,1.f);
  }

  Renderer::~Renderer(void) {
    this->destroyGBuffer();
    this->destroyPlain();
  }

  void Renderer::resize(int w, int h) {
    this->destroyGBuffer();
    this->initGBuffer(w, h);
  }
#undef OGL_NAME
}


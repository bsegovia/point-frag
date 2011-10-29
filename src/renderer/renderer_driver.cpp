#include "renderer_driver.hpp"
#include "sys/logging.hpp"

#include <cstring>

namespace pf
{
#define OGL_NAME this

  /*! Where you may find data files and shaders */
  const char *defaultPath[] = {
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
  const size_t defaultPathNum = ARRAY_ELEM_NUM(defaultPath);

  GLuint RendererDriver::makeGBufferTexture(int internal, int w, int h, int data, int type) {
    GLuint tex = 0;
    R_CALL (GenTextures, 1, &tex);
    R_CALL (BindTexture, GL_TEXTURE_2D, tex);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    R_CALL (TexImage2D, GL_TEXTURE_2D, 0, internal, w, h, 0, data, type, NULL);
    return tex;
  }

  GLuint RendererDriver::buildProgram(const std::string &prefix,
                                bool useVertex,
                                bool useGeometry,
                                bool useFragment)
  {
    GLuint program = 0, fragmentName = 0, vertexName = 0, geometryName = 0;
    R_CALLR (program, CreateProgram);

    if (useVertex) {
      const FileName path = FileName(prefix + ".vert");
      R_CALLR (vertexName, createShader, GL_VERTEX_SHADER, path);
      FATAL_IF (vertexName == 0, "Unable to create program");
      R_CALL (AttachShader, program, vertexName);
      R_CALL (DeleteShader, vertexName);
    }
    if (useFragment) {
      const FileName path = FileName(prefix + ".frag");
      R_CALLR (fragmentName, createShader, GL_FRAGMENT_SHADER, path);
      FATAL_IF (fragmentName == 0, "Unable to create program");
      R_CALL (AttachShader, program, fragmentName);
      R_CALL (DeleteShader, fragmentName);
    }
    if (useGeometry) {
      const FileName path = FileName(prefix + ".geom");
      R_CALLR (geometryName, createShader, GL_GEOMETRY_SHADER, path);
      FATAL_IF (geometryName == 0, "Unable to create program");
      R_CALL (AttachShader, program, geometryName);
      R_CALL (DeleteShader, geometryName);
    }
    R_CALL (LinkProgram, program);
    this->validateProgram(program);
    return program;
  }

  GLuint RendererDriver::buildProgram(const char *vertSource,
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

  void RendererDriver::initGBuffer(int w, int h) {
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
  void RendererDriver::destroyGBuffer(void) {
    if (this->gbuffer.fbo) R_CALL (DeleteFramebuffers, 1, &this->gbuffer.fbo);
    if (this->gbuffer.diffuse_exp) R_CALL (DeleteTextures, 1, &this->gbuffer.diffuse_exp);
    if (this->gbuffer.radiance) R_CALL (DeleteTextures, 1, &this->gbuffer.radiance);
    if (this->gbuffer.pxpypz_nx) R_CALL (DeleteTextures, 1, &this->gbuffer.pxpypz_nx);
    if (this->gbuffer.vxvy_nynz) R_CALL (DeleteTextures, 1, &this->gbuffer.vxvy_nynz);
    if (this->gbuffer.zbuffer) R_CALL (DeleteTextures, 1, &this->gbuffer.zbuffer);
  }

  /*! Plain shader -> display bounding boxex, wireframe geometries ...  */
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
  void RendererDriver::initPlain(void) {
    this->plain.program = buildProgram(plainVert, NULL, plainFrag);
    R_CALL (BindAttribLocation, this->plain.program, ATTR_POSITION, "Position");
    R_CALL (GenBuffers, 1, &this->plain.arrayBuffer);
    R_CALL (GenVertexArrays, 1, &this->plain.vertexArray);
    R_CALL (BindVertexArray, this->plain.vertexArray);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->plain.arrayBuffer);
    R_CALL (EnableVertexAttribArray, ATTR_POSITION);
    R_CALL (VertexAttribPointer, ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), NULL);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
    R_CALL (BindVertexArray, 0);
    R_CALLR (this->plain.uMVP, GetUniformLocation, this->plain.program, "MVP");
    R_CALLR (this->plain.uCol, GetUniformLocation, this->plain.program, "c");
  }
  void RendererDriver::destroyPlain(void) {
    R_CALL (DeleteProgram, this->plain.program);
    R_CALL (DeleteVertexArrays, 1, &this->plain.vertexArray);
    R_CALL (DeleteBuffers, 1, &this->plain.arrayBuffer);
  }

  /*! Diffuse shader -> just display geometries with a diffuse texture */
  static const char diffuseVert[] = {
    "#version 330 core\n"
    "#define POSITION 0\n"
    "#define TEXCOORD 1\n"
    "uniform mat4 MVP;\n"
    "layout(location = POSITION) in vec3 p;\n"
    "layout(location = TEXCOORD) in vec2 t;\n"
    "out block {\n"
    "  vec2 t;\n"
    "} Out;\n"
    "void main() {	\n"
    "  Out.t = t;\n"
    "  gl_Position = MVP * vec4(p.x, -p.y, p.z, 1.f);\n"
    "}\n"
  };
  static const char diffuseFrag[] = {
    "#version 330 core\n"
    "#define FRAG_COLOR  0\n"
    "uniform sampler2D Diffuse;\n"
    "in block {\n"
    "  vec2 t;\n"
    "} In;\n"
    "layout(location = FRAG_COLOR, index = 0) out vec4 c;\n"
    "void main() {\n"
    "  c = texture(Diffuse, In.t);\n"
    "}\n"
  };
  void RendererDriver::initDiffuse(void) {
    this->diffuse.program = buildProgram(diffuseVert, NULL, diffuseFrag);
    R_CALL (BindAttribLocation, this->diffuse.program, ATTR_POSITION, "Position");
    R_CALL (BindAttribLocation, this->diffuse.program, ATTR_TEXCOORD, "Texcoord");
    R_CALLR (this->diffuse.uMVP, GetUniformLocation, this->diffuse.program, "MVP");
    R_CALLR (this->diffuse.uDiffuse, GetUniformLocation, this->diffuse.program, "Diffuse");
    R_CALL (UseProgram, this->diffuse.program);
    R_CALL (Uniform1i, this->diffuse.uDiffuse, 0);
    R_CALL (UseProgram, 0);
  }
  void RendererDriver::destroyDiffuse(void) {
    R_CALL (DeleteProgram, this->diffuse.program);
  }

  /*! Quad -> mostly used to display textures or for simple deferred shading */
  static const char quadVert[] = {
    "#version 330 core\n"
    "#define POSITION	0\n"
    "#define TEXCOORD	1\n"
    "#define COLOR		3\n"
    "#define FRAG_COLOR	0\n"
    "uniform mat4 MVP;\n"
    "layout(location = POSITION) in vec2 p;\n"
    "layout(location = TEXCOORD) in vec2 t;\n"
    "out block\n"
    "{\n"
    "  vec2 t;\n"
    "} Out;\n"
    "void main()\n"
    "{	\n"
    "  Out.t = t;\n"
    "  gl_Position = MVP * vec4(p,0.f,1.f);\n"
    "}\n"
  };
  static const char quadFrag[] = {
    "#version 330 core\n"
    "#define POSITION	0\n"
    "#define TEXCOORD	1\n"
    "#define COLOR		3\n"
    "#define FRAG_COLOR	0\n"
    "uniform sampler2D Diffuse;\n"
    "in block\n"
    "{\n"
    "	vec2 t;\n"
    "} In;\n"
    "layout(location = FRAG_COLOR, index = 0) out vec4 c;\n"
    "void main()\n"
    "{\n"
    "  c = texture(Diffuse, In.t);\n"
    "  c /= c.w;\n"
    "  c.xyz = vec3(abs(dot(normalize(c.xyz), normalize(vec3(1,1,1)))));\n"
    "  //c = vec4(In.t.x,In.t.y,0,0);\n"
    "}\n"
  };
  struct QuadVertex {
    INLINE QuadVertex(vec2f p_, vec2f t_) : p(p_), t(t_) {}
    vec2f p, t;
  };
  static const QuadVertex quadData[] = {
    QuadVertex(vec2f(-1.0f,-1.0f), vec2f(0.0f, 1.0f)),
    QuadVertex(vec2f( 1.0f,-1.0f), vec2f(1.0f, 1.0f)),
    QuadVertex(vec2f( 1.0f, 1.0f), vec2f(1.0f, 0.0f)),
    QuadVertex(vec2f( 1.0f, 1.0f), vec2f(1.0f, 0.0f)),
    QuadVertex(vec2f(-1.0f, 1.0f), vec2f(0.0f, 0.0f)),
    QuadVertex(vec2f(-1.0f,-1.0f), vec2f(0.0f, 1.0f))
  };
  void RendererDriver::initQuad(void) {
    // Build vertex data
    R_CALL (GenBuffers, 1, &this->quad.arrayBuffer);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->quad.arrayBuffer);
    R_CALL (BufferData, GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
    R_CALL (GenVertexArrays, 1, &this->quad.vertexArray);
    R_CALL (BindVertexArray, this->quad.vertexArray);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->quad.arrayBuffer);
    R_CALL (VertexAttribPointer, ATTR_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), NULL);
    R_CALL (VertexAttribPointer, ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex,t));
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
    R_CALL (EnableVertexAttribArray, ATTR_POSITION);
    R_CALL (EnableVertexAttribArray, ATTR_TEXCOORD);
    R_CALL (BindVertexArray, 0);

    // Setup the program
    this->quad.program = buildProgram(quadVert, NULL, quadFrag);
    R_CALL (BindAttribLocation, this->quad.program, ATTR_POSITION, "p");
    R_CALL (BindAttribLocation, this->quad.program, ATTR_TEXCOORD, "t");
    R_CALLR (this->quad.uDiffuse, GetUniformLocation, this->quad.program, "Diffuse");
    R_CALLR (this->quad.uMVP, GetUniformLocation, this->quad.program, "MVP");
    R_CALL (UseProgram, this->quad.program);
    R_CALL (Uniform1i, this->quad.uDiffuse, 0);
    R_CALL (UseProgram, 0);
  }
  void RendererDriver::destroyQuad(void) {
    R_CALL (DeleteProgram, this->quad.program);
    R_CALL (DeleteVertexArrays, 1, &this->quad.vertexArray);
    R_CALL (DeleteBuffers, 1, &this->quad.arrayBuffer);
  }

  /*! Bounding box indices */
  static const int bboxIndex[24] = {
    0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 1,5, 2,6, 0,4, 3,7
  };
  static const size_t bboxIndexNum = ARRAY_ELEM_NUM(bboxIndex);

  void RendererDriver::displayBBox(const BBox3f *bbox, int n, const vec4f *col)
  {
    if (n == 0) return;

    // Build the vertex and the index buffers. We should proper instancing later
    // with some geometry shader to avoid that crap
    vec3f *pts = PF_NEW_ARRAY(vec3f, 8*n);
    int *indices = PF_NEW_ARRAY(int, bboxIndexNum*n);
    for (int i = 0; i < n; ++i) {
      pts[8*i+0].x = pts[8*i+3].x = pts[8*i+4].x = pts[8*i+7].x = bbox[i].lower.x;
      pts[8*i+0].y = pts[8*i+1].y = pts[8*i+4].y = pts[8*i+5].y = bbox[i].lower.y;
      pts[8*i+0].z = pts[8*i+1].z = pts[8*i+2].z = pts[8*i+3].z = bbox[i].lower.z;
      pts[8*i+1].x = pts[8*i+2].x = pts[8*i+6].x = pts[8*i+5].x = bbox[i].upper.x;
      pts[8*i+2].y = pts[8*i+3].y = pts[8*i+6].y = pts[8*i+7].y = bbox[i].upper.y;
      pts[8*i+4].z = pts[8*i+5].z = pts[8*i+6].z = pts[8*i+7].z = bbox[i].upper.z;
    }
    for (int i = 0; i < n; ++i)
    for (size_t j = 0; j < bboxIndexNum; ++j)
      indices[bboxIndexNum*i+j] = 8*i + bboxIndex[j];

    // Setup the draw and draw the boxes
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->plain.arrayBuffer);
    R_CALL (BufferData, GL_ARRAY_BUFFER, 8*n*sizeof(vec3f), pts, GL_STREAM_DRAW);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
    R_CALL (UseProgram, this->plain.program);
    R_CALL (BindVertexArray, this->plain.vertexArray);
    R_CALL (UniformMatrix4fv, this->plain.uMVP, 1, GL_FALSE, &this->MVP[0][0]);
    R_CALL (Disable, GL_CULL_FACE);
    R_CALL (Uniform4fv, this->plain.uCol, 1, &this->defaultDiffuseCol.x);
    R_CALL (DrawElements, GL_LINES, n*bboxIndexNum, GL_UNSIGNED_INT, indices);
    R_CALL (BindVertexArray, 0);
    R_CALL (Enable, GL_CULL_FACE);
    R_CALL (UseProgram, 0);
    PF_DELETE_ARRAY(pts);
    PF_DELETE_ARRAY(indices);
  }

  RendererDriver::RendererDriver(void) {
    PF_MSG_V("RendererDriver: initialization");
    std::memset(&this->gbuffer, 0, sizeof(this->gbuffer));
    this->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    this->initQuad();
    this->initPlain();
    this->initDiffuse();
    this->initGBuffer(16, 16);
    this->defaultDiffuseCol = this->defaultSpecularCol = vec4f(1.f,0.f,0.f,1.f);
  }

  RendererDriver::~RendererDriver(void) {
    PF_MSG_V("RendererDriver: destruction");
    this->destroyGBuffer();
    this->destroyDiffuse();
    this->destroyPlain();
    this->destroyQuad();
  }

  void RendererDriver::resize(int w, int h) {
    this->destroyGBuffer();
    this->initGBuffer(w, h);
  }

#undef OGL_NAME
} /* namespace pf */


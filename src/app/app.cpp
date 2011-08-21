#include "camera.hpp"
#include "pointcloud.hpp"
#include "texture.hpp"
#include "renderer/ogl.hpp"
#include "models/obj.hpp"
#include "math/vec.hpp"
#include "math/matrix.hpp"
#include "math/bbox.hpp"

#include <GL/freeglut.h>
#include <unordered_map>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sys/time.h>

using namespace pf;

static int w = 1024;
static int h = 1024;
static Obj *obj = NULL;
static const GLuint ATTRIB_POSITION = 0;
static const GLuint ATTRIB_TEXCOORD = 1;
static const GLuint ATTRIB_COLOR = 2;
static const GLuint ATTRIB_NORMAL = 3;
static const GLuint ATTRIB_TANGENT = 4;
static const size_t MAX_KEYS = 256;
static bool keys[MAX_KEYS];
static const std::string dataPath = "../../share/";

// All textures per object
static GLuint *texName = NULL;
// Bounding boxes of each object
static BBox3f *bbox = NULL;

// To display the geometry with a diffuse texture (only)
static GLuint diffuseOnlyProgram = 0;
static GLint uDiffuseOnlyMVP = 0;
static GLint uDiffuseOnlyTex = 0;

// To display the bounding boxes
static GLuint plainColorProgram = 0;
static GLint uPlainColorMVP = 0;
static GLint uPlainColorCol = 0;

// OBJ geometry
static GLuint ObjDataBufferName = 0;
static GLuint ObjElementBufferName = 0;
static GLuint ObjVertexArrayName = 0;
static GLuint chessTextureName = 0;

// Framebuffer (object)
static GLuint frameBufferName = 0;
static GLuint bufferRGBAName = 0;
static GLuint bufferDepthName = 0;

// Quat to display the final picture
static GLuint quadVertexArrayName = 0;
static GLuint quadBufferName = 0;
static GLuint quadProgramName = 0;
static GLuint quadUniformDiffuse = 0;
static GLuint quadUniformMVP = 0;

struct Vertex
{
  Vertex (vec2f p_, vec2f t_) : p(p_), t(t_) {}
  vec2f p,t;
};

const GLsizei VertexCount = 6;
const GLsizeiptr VertexSize = VertexCount * sizeof(Vertex);
const Vertex VertexData[VertexCount] =
{
  Vertex(vec2f(-1.0f,-1.0f), vec2f(0.0f, 1.0f)),
  Vertex(vec2f( 1.0f,-1.0f), vec2f(1.0f, 1.0f)),
  Vertex(vec2f( 1.0f, 1.0f), vec2f(1.0f, 0.0f)),
  Vertex(vec2f( 1.0f, 1.0f), vec2f(1.0f, 0.0f)),
  Vertex(vec2f(-1.0f, 1.0f), vec2f(0.0f, 0.0f)),
  Vertex(vec2f(-1.0f,-1.0f), vec2f(0.0f, 1.0f))
};

static FlyCamera cam;

static const float speed = 1.f; //  [m/s]
static const float angularSpeed = 4.f * 180.f / M_PI / 100.f; // [radian/pixel/s]
static double prevTime = 0., displayTime = 0.;
static int mouseX = 0, mouseY = 0;
static int mouseXRel = 0, mouseYRel = 0;
static bool isMouseInit = false;
static uint64_t currFrame = 0;

// Create the frame buffer object we use for the splats
static void buildFrameBufferObject(void)
{
  if (bufferRGBAName)
    GL_CALL (DeleteTextures, 1, &bufferRGBAName);
  if (bufferDepthName)
    GL_CALL (DeleteTextures, 1, &bufferDepthName);
  if (frameBufferName)
    GL_CALL (DeleteFramebuffers, 1, &frameBufferName);

  // Color buffer
  GL_CALL (ActiveTexture, GL_TEXTURE0);
  GL_CALL (GenTextures, 1, &bufferRGBAName);
  GL_CALL (BindTexture, GL_TEXTURE_2D, bufferRGBAName);
  GL_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  GL_CALL (TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  GL_CALL (TexImage2D, GL_TEXTURE_2D, 
                       0,
                       GL_RGBA16F,
                       w,
                       h,
                       0,
                       GL_RGBA, 
                       GL_UNSIGNED_BYTE, 
                       NULL);

  // Depth buffer
  GL_CALL (GenTextures, 1, &bufferDepthName);
  GL_CALL (BindTexture, GL_TEXTURE_2D, bufferDepthName);
  GL_CALL (TexImage2D, GL_TEXTURE_2D,
                       0,
                       GL_DEPTH24_STENCIL8,
                       w,
                       h,
                       0,
                       GL_DEPTH_STENCIL,
                       GL_UNSIGNED_INT_24_8,
                       NULL);

  GL_CALL (GenFramebuffers, 1, &frameBufferName);
  GL_CALL (BindFramebuffer, GL_FRAMEBUFFER, frameBufferName);
  GL_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufferRGBAName, 0);
  GL_CALL (FramebufferTexture, GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, bufferDepthName, 0);
  GL_CALL (BindFramebuffer, GL_FRAMEBUFFER, 0);
}

// Returns time elapsed from the previous frame
static double updateTime(void)
{
  struct timeval tval;
  gettimeofday(&tval, NULL);
  const double currTime = tval.tv_sec + 1e-6 * tval.tv_usec;
  const double dt = currTime - prevTime;

  // Display FPS with enough frames only
  if (currFrame % 100 == 0) {
    const double dt = currTime-displayTime;
    printf("time %lf msec fps %lf\n", dt * 1000. / 100., 100. / dt);
    displayTime = currTime;
  }

  prevTime = currTime; 
  currFrame++;
  return dt;
}

static void updateCamera(void)
{
  if (keys[27] == true) exit(0);

  // Get the time elapsed
  const double dt = updateTime();

  // Change mouse orientation
  const float xrel = float(mouseXRel) * dt * angularSpeed;
  const float yrel = float(mouseYRel) * dt * angularSpeed;
  cam.updateOrientation(xrel, yrel);
  mouseXRel = mouseYRel = 0;

  // Change mouse position
  vec3f d(0.f);
  if (keys['w']) d.z += float(dt) * speed;
  if (keys['s']) d.z -= float(dt) * speed;
  if (keys['a']) d.x += float(dt) * speed;
  if (keys['d']) d.x -= float(dt) * speed;
  if (keys['r']) d.y += float(dt) * speed;
  if (keys['f']) d.y -= float(dt) * speed;
  cam.updatePosition(d);

  // Compute the MVP (Model View Projection matrix)
  cam.ratio = float(w) / float(h);
}

static void display(void)
{
  updateCamera();
  const mat4x4f MVP = cam.getMatrix();

  // Set the display viewport
  GL_CALL (Viewport, 0, 0, w, h);
  GL_CALL (Enable, GL_DEPTH_TEST);
  GL_CALL (Enable, GL_CULL_FACE);
  GL_CALL (CullFace, GL_FRONT);
  GL_CALL (DepthFunc, GL_LEQUAL);

  // Clear color buffer with black
  GL_CALL (ClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
  GL_CALL (ClearDepth, 1.0f);
  GL_CALL (Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Display the objects with their textures
  GL_CALL (UseProgram, diffuseOnlyProgram);
  GL_CALL (UniformMatrix4fv, uDiffuseOnlyMVP, 1, GL_FALSE, &MVP[0][0]);
  GL_CALL (BindVertexArray, ObjVertexArrayName);
  GL_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, ObjElementBufferName);
  GL_CALL (ActiveTexture, GL_TEXTURE0);
  for (size_t grp = 0; grp < obj->grpNum; ++grp) {
    const uintptr_t offset = obj->grp[grp].first * sizeof(int[3]);
    const GLuint n = obj->grp[grp].last - obj->grp[grp].first + 1;
    GL_CALL (BindTexture, GL_TEXTURE_2D, texName[grp]);
    GL_CALL (DrawElements, GL_TRIANGLES, 3*n, GL_UNSIGNED_INT, (void*) offset);
  }
  GL_CALL (BindVertexArray, 0);
  GL_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);

  // Display the object bounding boxes
  GL_CALL (UseProgram, plainColorProgram);
  GL_CALL (UniformMatrix4fv, uPlainColorMVP, 1, GL_FALSE, &MVP[0][0]);
  GL_CALL (Uniform4f, uPlainColorCol, 1.f, 0.f, 0.f, 1.f);
  // Build the indices
  const int index[36] = {
    0,3,1, 1,3,2, 4,5,7, 5,6,7,
    5,1,2, 5,2,6, 0,3,4, 3,4,7,
    2,3,7, 2,7,6, 1,4,5, 0,1,3
  };

  vec3f pts[8]; // 8 points of the bounding box
  GL_CALL (Disable, GL_CULL_FACE);
  GL_CALL (PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
  for (size_t grp = 0; grp < obj->grpNum; ++grp) {
    pts[0].x = pts[3].x = pts[4].x = pts[7].x = bbox[grp].lower.x;
    pts[0].y = pts[1].y = pts[4].y = pts[5].y = bbox[grp].lower.y;
    pts[0].z = pts[1].z = pts[2].z = pts[3].z = bbox[grp].lower.z;
    pts[1].x = pts[2].x = pts[6].x = pts[5].x = bbox[grp].upper.x;
    pts[2].y = pts[3].y = pts[6].y = pts[7].y = bbox[grp].upper.y;
    pts[4].z = pts[5].z = pts[6].z = pts[7].z = bbox[grp].upper.z;
    GL_CALL (EnableVertexAttribArray, ATTRIB_POSITION);
    GL_CALL (VertexAttribPointer, ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), &pts[0].x);
    GL_CALL (DrawElements, GL_TRIANGLES, 36, GL_UNSIGNED_INT, index);
  }
  GL_CALL (DisableVertexAttribArray, ATTRIB_POSITION);
  GL_CALL (Enable, GL_CULL_FACE);
  GL_CALL (UseProgram, 0);
  GL_CALL (PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
  glutSwapBuffers();
}

static void buildObjBuffers(const Obj *obj)
{
  const size_t vertexSize = obj->vertNum * sizeof(ObjVertex);

  // Contain the vertex data
  GL_CALL (GenBuffers, 1, &ObjDataBufferName);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, ObjDataBufferName);
  GL_CALL (BufferData, GL_ARRAY_BUFFER, vertexSize, obj->vert, GL_STATIC_DRAW);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);

  // Build the indices
  GLuint *indices = new GLuint[obj->triNum * 3];
  const size_t indexSize = sizeof(GLuint[3]) * obj->triNum;
  for (size_t from = 0, to = 0; from < obj->triNum; ++from, to += 3) {
    indices[to+0] = obj->tri[from].v[0];
    indices[to+1] = obj->tri[from].v[1];
    indices[to+2] = obj->tri[from].v[2];
  }
  GL_CALL (GenBuffers, 1, &ObjElementBufferName);
  GL_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, ObjElementBufferName);
  GL_CALL (BufferData, GL_ELEMENT_ARRAY_BUFFER, indexSize, indices, GL_STATIC_DRAW);
  GL_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
  delete [] indices;

  // Contain the vertex array
  GL_CALL (GenVertexArrays, 1, &ObjVertexArrayName);
  GL_CALL (BindVertexArray, ObjVertexArrayName);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, ObjDataBufferName);
  GL_CALL (VertexAttribPointer, ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), NULL);
  GL_CALL (VertexAttribPointer, ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)(6*sizeof(float)));
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
  GL_CALL (EnableVertexAttribArray, ATTRIB_POSITION);
  GL_CALL (EnableVertexAttribArray, ATTRIB_TEXCOORD);
  GL_CALL (BindVertexArray, 0);
}

static void buildQuadBuffer(void)
{
  GL_CALL (GenBuffers, 1, &quadBufferName);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, quadBufferName);
  GL_CALL (BufferData, GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STATIC_DRAW);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
  GL_CALL (GenVertexArrays, 1, &quadVertexArrayName);
  GL_CALL (BindVertexArray, quadVertexArrayName);
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, quadBufferName);
  GL_CALL (VertexAttribPointer, ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
  GL_CALL (VertexAttribPointer, ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,t));
  GL_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
  GL_CALL (EnableVertexAttribArray, ATTRIB_POSITION);
  GL_CALL (EnableVertexAttribArray, ATTRIB_TEXCOORD);
  GL_CALL (BindVertexArray, 0);
}

static void keyDown(unsigned char key, int x, int y) { keys[key] = true; }
static void keyUp(unsigned char key, int x, int y) { keys[key] = false; }
static void mouse(int Button, int State, int x, int y) { }
static void reshape(int _w, int _h) {
  w = _w; h = _h;
  buildFrameBufferObject();
}
static void idle(void) { glutPostRedisplay(); }

static void entry(int state)
{
  if (state == GLUT_LEFT) {
//    for (size_t i = 0; i < MAX_KEYS; ++i)
//      keys[i] = false;
    glutWarpPointer(w/2,h/2);
    isMouseInit = false;
  }
}

static void motion(int x, int y)
{
  if (isMouseInit == false) {
    mouseX = x;
    mouseY = y;
    mouseYRel = mouseXRel = 0;
    isMouseInit = true;
    return;
  }
  mouseXRel = x - mouseX;
  mouseYRel = y - mouseY;
  mouseX = x;
  mouseY = y;
}

static GLuint buildProgram(const std::string &prefix,
                           bool useVertex,
                           bool useGeometry,
                           bool useFragment)
{
  GLuint program = 0, fragmentName = 0, vertexName = 0, geometryName = 0;

  if (useVertex) {
    const std::string vertexSrc = prefix + ".vert";
    GL_CALLR (vertexName, createShader, GL_VERTEX_SHADER, vertexSrc.c_str());
  }
  if (useFragment) {
    const std::string fragmentSrc = prefix + ".frag";
    GL_CALLR (fragmentName, createShader, GL_FRAGMENT_SHADER, fragmentSrc.c_str());
  }
  if (useGeometry) {
    const std::string geometrySrc = prefix + ".geom";
    GL_CALLR (geometryName, createShader, GL_GEOMETRY_SHADER, geometrySrc.c_str());
  }

  GL_CALLR (program, CreateProgram);
  if (vertexName) {
    GL_CALL (AttachShader, program, vertexName);
    GL_CALL (DeleteShader, vertexName);
  }
  if (geometryName) {
    GL_CALL (AttachShader, program, geometryName);
    GL_CALL (DeleteShader, geometryName);
  }
  if (fragmentName) {
    GL_CALL (AttachShader, program, fragmentName);
    GL_CALL (DeleteShader, fragmentName);
  }
  GL_CALL (LinkProgram, program);
  return program;
}

static void buildDiffuseMesh(void)
{
  // Store the texture for each fileName
  std::unordered_map<std::string, GLuint> texMap;

  // chess texture is used when no diffuse is bound
  const std::string name = dataPath + "Maps/chess.tga";
  chessTextureName = texMap["chess.tga"] = loadTexture(name.c_str());

  // Load all textures
  for (size_t i = 0; i < obj->matNum; ++i) {
    const char *name = obj->mat[i].map_Kd;
    if (name == NULL || strlen(name) == 0)
      continue;
    if (texMap.find(name) == texMap.end()) {
      const std::string path = dataPath + name;
      const GLuint texName = loadTexture(path.c_str());
      texMap[name] = texName ? texName : chessTextureName;
    }
    else
      printf((std::string(name) + " already found\n").c_str());
  }

  // GLSL program to display geometries with diffuse
  diffuseOnlyProgram = buildProgram(dataPath + "texture", true, false, true);
  GL_CALL (BindAttribLocation, diffuseOnlyProgram, ATTRIB_POSITION, "Position");
  GL_CALL (BindAttribLocation, diffuseOnlyProgram, ATTRIB_TEXCOORD, "Texcoord");
  GL_CALLR (uDiffuseOnlyMVP, GetUniformLocation, diffuseOnlyProgram, "MVP");
  GL_CALLR (uDiffuseOnlyTex, GetUniformLocation, diffuseOnlyProgram, "Diffuse");
  GL_CALL (UseProgram, diffuseOnlyProgram);
  GL_CALL (Uniform1i, uDiffuseOnlyTex, 0);
  GL_CALL (UseProgram, 0);

  // GLSL program to display geometries with plain color
  plainColorProgram = buildProgram(dataPath + "plain", true, false, true);
  GL_CALL (BindAttribLocation, plainColorProgram, ATTRIB_POSITION, "Position");
  GL_CALLR (uPlainColorMVP, GetUniformLocation, plainColorProgram, "MVP");
  GL_CALLR (uPlainColorCol, GetUniformLocation, plainColorProgram, "c");

  buildObjBuffers(obj);

  // Map each material group to the texture name
  texName = new GLuint[obj->grpNum];
  for (size_t i = 0; i < obj->grpNum; ++i) {
    const int mat = obj->grp[i].m;
    const char *name = obj->mat[mat].map_Kd;
    if (name == NULL || strlen(name) == 0)
      texName[i] = chessTextureName;
    else {
      texName[i] = texMap[name];
      printf("name %s %i\n", name, (int)i);
    }
  }

  // Compute each object bounding box
  bbox = new BBox3f[obj->grpNum];
  for (size_t i = 0; i < obj->grpNum; ++i) {
    const size_t first = obj->grp[i].first, last = obj->grp[i].last;
    bbox[i] = BBox3f(empty);
    for (size_t j = first; j < last; ++j) {
      const ObjVertex &v0 = obj->vert[obj->tri[j].v[0]];
      const ObjVertex &v1 = obj->vert[obj->tri[j].v[1]];
      const ObjVertex &v2 = obj->vert[obj->tri[j].v[2]];
      bbox[i].grow(v0.p);
      bbox[i].grow(v1.p);
      bbox[i].grow(v2.p);
    }
  }
}

static void buildQuad(void)
{
  // screen aligned quad
  buildQuadBuffer();

  // Load the GLSL program to display geometries with diffuse
  quadProgramName = buildProgram(dataPath + "deferred", true, false, true);
  GL_CALL (BindAttribLocation, quadProgramName, ATTRIB_POSITION, "p");
  GL_CALL (BindAttribLocation, quadProgramName, ATTRIB_TEXCOORD, "t");

  // Get the uniforms
  GL_CALLR (quadUniformDiffuse, GetUniformLocation, quadProgramName, "Diffuse");
  GL_CALLR (quadUniformMVP, GetUniformLocation, quadProgramName, "MVP");
  GL_CALL (UseProgram, quadProgramName);
  GL_CALL (Uniform1i, quadUniformDiffuse, 0);
  GL_CALL (UseProgram, 0);
}

int main(int argc, char **argv)
{
  glutInitWindowSize(w, h);
  glutInitWindowPosition(64, 64);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
  glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutCreateWindow(argv[0]);

  ogl = new struct ogl;

  // Load OBJ file
  obj = new Obj;
  FATAL_IF (obj->load((dataPath + "f000.obj").c_str()) == false, "Loading failed");
  std::printf("loaded OBJ\n");
  std::printf("triNum: %u\n", (uint32_t) obj->triNum);
  std::printf("vertNum: %u\n", (uint32_t) obj->vertNum);
  std::printf("matNum: %u\n", (uint32_t) obj->matNum);
  std::printf("grpNum: %u\n", (uint32_t) obj->grpNum);

  buildFrameBufferObject();
  buildDiffuseMesh();
  glutDisplayFunc(display);
  buildQuad();

  struct timeval tval;
  gettimeofday(&tval, NULL);
  displayTime = prevTime = tval.tv_sec + 1e-6 * tval.tv_usec;

  glutIdleFunc(idle); 
  glutReshapeFunc(reshape);
  glutEntryFunc(entry);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(motion);
  glutKeyboardFunc(keyDown);
  glutKeyboardUpFunc(keyUp);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
  glutMainLoop();

  return 0;
}


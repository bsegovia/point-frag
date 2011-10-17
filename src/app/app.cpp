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

#include "camera.hpp"
#include "models/obj.hpp"
#include "math/vec.hpp"
#include "math/matrix.hpp"
#include "math/bbox.hpp"
#include "renderer/texture.hpp"
#include "renderer/renderobj.hpp"
#include "renderer/renderer.hpp"

#include <GL/freeglut.h>
#include <unordered_map>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>

using namespace pf;

static int w = 800, h = 600;
static const size_t MAX_KEYS = 256;
static bool keys[MAX_KEYS];
static Renderer *renderer = NULL;
Ref<RenderObj> renderObj = NULL;

#define OGL_NAME ((Renderer*)ogl)

static FlyCamera cam;

static const float speed = 1.f; //  [m/s]
static const float angularSpeed = 4.f * 180.f / float(pi) / 100.f; // [radian/pixel/s]
static double prevTime = 0., displayTime = 0.;
static int mouseX = 0, mouseY = 0;
static int mouseXRel = 0, mouseYRel = 0;
static bool isMouseInit = false;
static uint64_t currFrame = 0;

// Returns time elapsed from the previous frame
static double updateTime(void)
{
  const double currTime = getSeconds();
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
  if (keys[27] == true) {
    renderObj = NULL;
    PF_DELETE(renderer);
    ogl = renderer = NULL;
    dumpAlloc();
    exit(0);
  }

  // Get the time elapsed
  const double dt = updateTime();

  // Change mouse orientation
  const float xrel = float(mouseXRel) * float(dt) * angularSpeed;
  const float yrel = float(mouseYRel) * float(dt) * angularSpeed;
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
  R_CALL (Viewport, 0, 0, w, h);
  R_CALL (Enable, GL_DEPTH_TEST);
  R_CALL (Enable, GL_CULL_FACE);
  R_CALL (CullFace, GL_FRONT);
  R_CALL (DepthFunc, GL_LEQUAL);

  // Clear color buffer with black
  R_CALL (ClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
  R_CALL (ClearDepth, 1.0f);
  R_CALL (Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Display the objects with their textures
  R_CALL (UseProgram, renderer->diffuse.program);
  R_CALL (UniformMatrix4fv, renderer->diffuse.uMVP, 1, GL_FALSE, &MVP[0][0]);
  renderObj->display();
  R_CALL (UseProgram, 0);

  // Display all the bounding boxes
  R_CALL (setMVP, MVP);
  R_CALL (displayBBox, renderObj->bbox, renderObj->grpNum);
  glutSwapBuffers();
}

static void keyDown(unsigned char key, int x, int y) { keys[key] = true; }
static void keyUp(unsigned char key, int x, int y) { keys[key] = false; }
static void mouse(int Button, int State, int x, int y) { }
static void reshape(int _w, int _h) {
  w = _w; h = _h;
  renderer->resize(w, h);
}
static void idle(void) { glutPostRedisplay(); }

static void entry(int state)
{
  if (state == GLUT_LEFT) {
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

int main(int argc, char **argv)
{
  startMemoryDebugger();
  glutInitWindowSize(w, h);
  glutInitWindowPosition(64, 64);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
  glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutCreateWindow(argv[0]);

  ogl = renderer = PF_NEW(Renderer);
  renderObj = PF_NEW(RenderObj, *renderer, "f000.obj");

  displayTime = getSeconds();

  glutDisplayFunc(display);
  glutIdleFunc(idle); 
  glutReshapeFunc(reshape);
  glutEntryFunc(entry);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(motion);
  glutKeyboardFunc(keyDown);
  glutKeyboardUpFunc(keyUp);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

  while (keys[27] == 0) {
    glutMainLoopEvent();
    display();
  }

  PF_DELETE(renderer);
  ogl = renderer = NULL;
  return 0;
}
#undef OGL_NAME


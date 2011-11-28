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

#include "game_frame.hpp"
#include "sys/alloc.hpp"
#include "sys/tasking.hpp"
#include "sys/logging.hpp"
#include "renderer/renderer_obj.hpp"
#include "renderer/renderer.hpp"
#include "models/obj.hpp"

#include <GL/freeglut.h>
#include <cstdio>
#include <iostream>

#include "renderer/hiz.hpp" // XXX to test the HiZ
#include "rt/bvh2.hpp"      // XXX to test the HiZ
#include "rt/rt_triangle.hpp"// XXX to test the HiZ

namespace pf
{
  static const int defaultWidth = 1024, defaultHeight = 1024;
  Renderer *renderer = NULL;
  Ref<RendererObj> renderObj = NULL;
  static LoggerStream *coutStream = NULL;
  static LoggerStream *fileStream = NULL;

  class CoutStream : public LoggerStream {
  public:
    virtual LoggerStream& operator<< (const std::string &str) {
      std::cout << str;
      return *this;
    }
  };

  class FileStream : public LoggerStream {
  public:
    FileStream(void) {
      file = fopen("log.txt", "w");
      assert(file);
    }
    virtual ~FileStream(void) {fclose(file);}
    virtual LoggerStream& operator<< (const std::string &str) {
      fprintf(file, str.c_str());
      return *this;
    }
  private:
    FILE *file;
  };

  static void LoggerStart(void) {
    logger = PF_NEW(Logger);
    coutStream = PF_NEW(CoutStream);
    fileStream = PF_NEW(FileStream);
    logger->insert(*coutStream);
    logger->insert(*fileStream);
  }

  static void LoggerEnd(void) {
    logger->remove(*fileStream);
    logger->remove(*coutStream);
    PF_DELETE(fileStream);
    PF_DELETE(coutStream);
    PF_DELETE(logger);
    logger = NULL;
  }

  //static const FileName objName("f000.obj");
  //static const FileName objName("small.obj");
  //static const FileName objName("Arabic_City.obj");
  static const FileName objName("arabic_city_II.obj");
  //static const FileName objName("conference.obj");
  //static const FileName objName("sibenik.obj");
  //static const FileName objName("sponza.obj");

  extern Ref<BVH2<RTTriangle>> bvh; // XXX -> HiZ

  static void GameStart(int argc, char **argv) {
    PF_MSG_V("GLUT: initialization");
    glutInitWindowSize(defaultWidth, defaultHeight);
    glutInitWindowPosition(64, 64);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    PF_MSG_V("GLUT: creating window");
    glutCreateWindow(argv[0]);

    renderer = PF_NEW(Renderer);

    size_t path = 0;
    Obj obj;
    for (path = 0; path < defaultPathNum; ++path)
      if (obj.load(FileName(defaultPath[path]) + objName)) {
        PF_MSG_V("Obj: " << objName << " loaded from " << defaultPath[path]);
        break;
      }
    if (path == defaultPathNum)
      PF_WARNING_V("Obj: " << objName << " not found");

    PF_MSG_V("RendererObj: building BVH of segments");
    RTTriangle *tris = PF_NEW_ARRAY(RTTriangle, obj.triNum);
    for (size_t i = 0; i < obj.triNum; ++i) {
      const vec3f &v0 = obj.vert[obj.tri[i].v[0]].p;
      const vec3f &v1 = obj.vert[obj.tri[i].v[1]].p;
      const vec3f &v2 = obj.vert[obj.tri[i].v[2]].p;
      tris[i] = RTTriangle(v0,v1,v2);
    }
    bvh = PF_NEW(BVH2<RTTriangle>);
    buildBVH2(tris, obj.triNum, *bvh);
    PF_DELETE_ARRAY(tris);

    // Build the renderer OBJ
    renderObj = PF_NEW(RendererObj, *renderer, obj);
  }

  static void GameEnd(void) {
    bvh = NULL; // release the BVH
    renderObj = NULL; // idem
    PF_DELETE(renderer);
    renderer = NULL;
  }
} /* namespace pf */

int main(int argc, char **argv)
{
  using namespace pf;
  MemDebuggerStart();
  TaskingSystemStart();
  LoggerStart();
  GameStart(argc, argv);

  // We create a dummy frame such that a previous frame always exists in the
  // system. This makes everything a lot easier to handle. We do not destroy it
  // since it is referenced counted
  GameFrame *dummyFrame = PF_NEW(GameFrame, defaultWidth, defaultHeight);
  Task *frameTask = PF_NEW(TaskGameFrame, *dummyFrame);
  frameTask->scheduled();
  TaskingSystemEnter();

  GameEnd();
  LoggerEnd();
  TaskingSystemEnd();
  MemDebuggerDumpAlloc();
  return 0;
}

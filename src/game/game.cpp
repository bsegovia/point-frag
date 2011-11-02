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

#include <GL/freeglut.h>
#include <cstdio>
#include <iostream>

using namespace pf;

namespace pf
{
  static const int defaultWidth = 800, defaultHeight = 600;
  Renderer *renderer = NULL;
  Ref<RendererObj> renderObj = NULL;
  LoggerStream *coutStream = NULL;
  LoggerStream *fileStream = NULL;

  class CoutStream : public LoggerStream
  {
    public:
      virtual LoggerStream& operator<< (const std::string &str) {
        std::cout << str;
        return *this;
      }
  };

  class FileStream : public LoggerStream
  {
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

  void LoggerStart(void)
  {
    logger = PF_NEW(Logger);
    coutStream = PF_NEW(CoutStream);
    fileStream = PF_NEW(FileStream);
    logger->insert(*coutStream);
    logger->insert(*fileStream);
  }

  void LoggerEnd(void)
  {
    logger->remove(*fileStream);
    logger->remove(*coutStream);
    PF_DELETE(fileStream);
    PF_DELETE(coutStream);
    PF_DELETE(logger);
    logger = NULL;
  }

  void GameStart(int argc, char **argv)
  {
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
    renderObj = PF_NEW(RendererObj, *renderer, "f000.obj");
  }

  void GameEnd()
  {
    renderObj = NULL;
    PF_DELETE(renderer);
    renderer = NULL;
  }
}

int main(int argc, char **argv)
{
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


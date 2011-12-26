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

#include "game.hpp"
#include "sys/logging.hpp"
#include "sys/alloc.hpp"
#include "sys/tasking.hpp"
#include "sys/string.hpp"
#include "utest/utest.hpp"

namespace pf
{
  static LoggerStream *coutStream = NULL;
  static LoggerStream *fileStream = NULL;

  class CoutStream : public LoggerStream {
  public:
    virtual LoggerStream& operator<< (const std::string &str) {
      std::cout << str;
      return *this;
    }
    PF_CLASS(CoutStream);
  };

  class FileStream : public LoggerStream {
  public:
    FileStream(void) {
      file = fopen("log.txt", "w");
      PF_ASSERT(file);
    }
    virtual ~FileStream(void) {fclose(file);}
    virtual LoggerStream& operator<< (const std::string &str) {
      fprintf(file, str.c_str());
      return *this;
    }
  private:
    FILE *file;
    PF_CLASS(FileStream);
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
} /* namespace pf */

int main(int argc, char *argv[])
{
  using namespace pf;
  MemDebuggerStart();
  TaskingSystemStart();
  LoggerStart();

  // Run the unit tests specified by the user
  if (argc > 1 && strequal(argv[1], "--utests")) {
    if (argc == 2)
      UTest::runAll();
    else for (int i = 2; i < argc; ++i)
      UTest::run(argv[i]);
  }
  // Run the game
  else
    game(argc, argv);

  LoggerEnd();
  TaskingSystemEnd();
  MemDebuggerDumpAlloc();
  return 0;
}

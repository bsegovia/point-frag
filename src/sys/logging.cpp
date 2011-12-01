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

#include "sys/logging.hpp"
#include "sys/tasking.hpp"
#include "sys/filename.hpp"

namespace pf
{
  LoggerStream::LoggerStream(void) : next(NULL) {}
  LoggerStream::~LoggerStream(void) {}

  LoggerBuffer::LoggerBuffer(void) : logger(NULL) {}

  LoggerBuffer& LoggerBuffer::operator<< (LoggerFlushTy) {
    logger->output(ss.str());
    ss.str("");
    return *this;
  }

  LoggerBuffer& LoggerBuffer::operator<< (const LoggerInfo &info) {
    FileName fileName(info.file);
    return *this << fileName.base() << " at " << info.function << " line " << info.line;
  }

  Logger::Logger(void) : streams(NULL) {
    const uint32 threadNum = TaskingSystemGetThreadNum();
    this->buffers = PF_NEW_ARRAY(LoggerBuffer, threadNum);
    for (uint32 i = 0; i < threadNum; ++i) this->buffers[i].logger = this;
    this->startTime = getSeconds();
  }

  Logger::~Logger(void) {
    FATAL_IF(streams != NULL, "You muyst remove all streams "
                              "before deleting the logger");
    PF_DELETE_ARRAY(buffers);
  }

  void Logger::output(const std::string &str) {
    Lock<MutexSys> lock(mutex);
    LoggerStream *stream = this->streams;
    while (stream) {
      *stream << str;
      stream = stream->next;
    }
  }

  void Logger::insert(LoggerStream &stream) {
    Lock<MutexSys> lock(mutex);
    stream.next = this->streams;
    this->streams = &stream;
  }

  void Logger::remove(LoggerStream &stream) {
    Lock<MutexSys> lock(mutex);
    LoggerStream *curr = this->streams;
    LoggerStream *pred = NULL;
    while (curr) {
      if (curr == &stream)
        break;
      pred = curr;
      curr = curr->next;
    }
    FATAL_IF (curr == NULL, "Unable to find the given stream");
    if (pred)
      pred->next = curr->next;
    else
      this->streams = curr->next;
  }

  Logger *logger = NULL;
} /* namespace pf */


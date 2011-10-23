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

#ifndef __PF_LOGGING_HPP__
#define __PF_LOGGING_HPP__

#include "sys/mutex.hpp"
#include "sys/tasking.hpp"
#include "sys/platform.hpp"
#include <sstream>
#include <iomanip>

namespace pf
{
  class Logger;
  class LoggerBuffer;
  class LoggerStream;

  /*! A logger stream is one way to output a string. It can be a file,
   *  stdout, the in-game console and so on...
   */
  class LoggerStream
  {
  public:
    LoggerStream(void);
    virtual LoggerStream& operator<< (const std::string &str) = 0;
  private:
    friend class Logger;
    LoggerStream *next; //!< We chain the logger elements together
  };

  /*! Helper and proxy structures to display various information */
  static struct LoggerFlushTy { } loggerFlush MAYBE_UNUSED;
  static struct LoggerThreadIDTy { } loggerThreadID MAYBE_UNUSED;
  struct LoggerInfo {
    INLINE LoggerInfo(const char *file, const char *function, int line) :
      file(file), function(function), line(line) {}
    const char *file, *function;
    int line;
  };

  /*! Used to lazily create strings from the user defined logs. When destroyed
   *  or flushed, it displays everything in one piece
   */
  class LoggerBuffer
  {
  public:
    template <typename T> LoggerBuffer& operator<< (const T &x) {
      ss << x;
      return *this;
    }
    LoggerBuffer& operator<< (LoggerFlushTy);
    LoggerBuffer& operator<< (LoggerThreadIDTy);
    LoggerBuffer& operator<< (const LoggerInfo&);
    /*! Output the info a nice format */
    LoggerBuffer& info(const char *file, const char *function, int line);
  private:
    friend class Logger;
    LoggerBuffer(void);
    Logger *logger;       //!< The logger that created this buffer
    std::stringstream ss; //!< Stores all the user strings
  };

  /*! Central class to log anything from the engine */
  class Logger
  {
  public:
    Logger(void);
    ~Logger(void);
    /*! Output the string into all the attached streams */
    void output(const std::string &str);
    template <typename T> LoggerBuffer& operator<< (const T &x) {
      LoggerBuffer &buffer = buffers[TaskingSystemGetThreadID()];
      buffer << "[" << std::setw(12) << std::fixed << getSeconds() - startTime << "s] " << x;
      return buffer;
    }
    void insert(LoggerStream &stream);
    void remove(LoggerStream &stream);
  private:
    MutexSys mutex;        //!< To insert / remove streams and output strings
    LoggerStream *streams; //!< All the output streams
    LoggerBuffer *buffers; //!< One buffer per thread
    double startTime;      //!< When the logger has been created
  };

  /*! We have one logger for the application */
  extern Logger *logger;

} /* namespace pf */

/*! Macros to handle logging information in the code */
#define PF_LOG_HERE LoggerInfo(__FILE__, __FUNCTION__, __LINE__)
#define PF_INFO " [thread " << loggerThreadID << " - " << PF_LOG_HERE << "]"

/*! Verbose macros: they add logging position and thread ID */
#define PF_WARNING_V(MSG) (*logger << "WARNING " << MSG << PF_INFO << "\n" << loggerFlush)
#define PF_ERROR_V(MSG) (*logger << "ERROR " << MSG << PF_INFO << "\n" << loggerFlush)
#define PF_MSG_V(MSG) (*logger << MSG << PF_INFO << "\n" << loggerFlush)

/*! Regular macros: just the user message */
#define PF_WARNING(MSG) (*logger << MSG << "\n" << loggerFlush;
#define PF_ERROR(MSG) (*logger << MSG << "\n" << loggerFlush;
#define PF_MSG(MSG) (*logger << MSG << "\n" << loggerFlush;

#endif /* __PF_LOGGING_HPP__ */


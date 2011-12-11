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

#ifndef __PF_TASKING_UTILITY_HPP__
#define __PF_TASKING_UTILITY_HPP__

#include "tasking.hpp"

namespace pf
{
  /*! Make the main thread return to the top-level function */
  class TaskInterruptMain : public Task
  {
  public:
    TaskInterruptMain(void);
    virtual Task *run(void);
  };

  /*! Does nothing but can be used for synchronization */
  class TaskDummy : public Task
  {
  public:
    TaskDummy(void);
    virtual Task *run(void);
  };

  /*! A task that runs on the main thread */
  class TaskMain : public Task
  {
  public:
    INLINE TaskMain(const char *name) : Task(name) {
      this->setAffinity(PF_TASK_MAIN_THREAD);
    }
  };

  /*! Encapsulates functor (and anonymous lambda) */
  template <typename T>
  class TaskFunctor : public Task
  {
  public:
    INLINE TaskFunctor(const T &functor, const char *name = NULL) :
      Task(name), functor(functor) {}
    virtual Task *run(void) { functor(); return NULL; }
  private:
    T functor;
  };

  /*! Spawn a task from a functor */
  template <typename T>
  Task *spawnTask(const char *name, const T &functor) {
    return PF_NEW(TaskFunctor<T>, functor, name);
  }

} /* namespace pf */

/*! To give a name to an anonymous task */
#define TASK_HERE (STRING(__LINE__) "@" __FILE__)

#endif /* __PF_TASKING_UTILITY_HPP__ */


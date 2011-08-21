// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
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

#ifndef __PF_TASKING_H__
#define __PF_TASKING_H__

#include "platform.hpp"
#include "thread.hpp"
#include "atomic.hpp"
#include "barrier.hpp"

#include <vector>

#define MAX_TASKS 100000

namespace pf
{
  class Task
  {
  public:

    typedef void (*runFunction)(size_t tid, void* ptr, size_t elt);
    typedef void (*completeFunction)(size_t tid, void* ptr);

    INLINE Task() : run(NULL), runData(NULL), complete(NULL), completeData(NULL) {}
    INLINE Task(runFunction run, void* runData, size_t elts, completeFunction complete, void* completeData)
      : run(run), runData(runData), complete(complete), completeData(completeData), started(elts), completed(elts) {}

  public:
    runFunction run;
    void* runData;
    completeFunction complete;
    void* completeData;
    Atomic started,completed;
  };

  class TaskScheduler
  {
    volatile bool terminateThreads;
    std::vector<thread_t> threads;
    Barrier barrier;

    /* workqueue */
    MutexSys taskMutex;
    Atomic numTasks;
    class Task* tasks[MAX_TASKS];
    Atomic activeTasks;

    struct Thread {
      Thread (size_t tid, TaskScheduler* scheduler) : tid(tid), scheduler(scheduler) {}
      size_t tid;
      TaskScheduler* scheduler;
    };

  public:
    TaskScheduler (size_t numThreads);
    ~TaskScheduler();

    void addTask(Task::runFunction run, void* runData, size_t elts = 1, Task::completeFunction complete = NULL, void* completeData = NULL);
    void go();
    void run(size_t tid);
    size_t getNumThreads() { return threads.size()+1; }

    static void init(int numThreads = -1);
    static void cleanup();
    static void threadFunction(Thread* thread);
  };

  extern TaskScheduler* scheduler;
}

#endif


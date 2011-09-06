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

#include "tasking.hpp"
#include <xmmintrin.h>
#include "sys/sysinfo.hpp"

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace pf
{
  TaskScheduler::TaskScheduler (size_t numThreads) : numTasks(0), activeTasks(0)
  {
    terminateThreads = false;
    setAffinity(0);
    barrier.init(numThreads);
    for (size_t i=0; i<numThreads-1; i++)
      threads.push_back(createThread((thread_func)threadFunction,NEW(Thread,i+1,this),4*1024*1024,int(i+1)));
    barrier.wait();
  }

  TaskScheduler::~TaskScheduler()
  {
    if (threads.size() == 0) return;
    terminateThreads = true;
    barrier.wait();
    for (size_t i=0; i<threads.size(); i++) join(threads[i]);
    threads.clear();
    terminateThreads = false;
  }

  void TaskScheduler::addTask(Task::runFunction run, void* runData, size_t elts, Task::completeFunction complete, void* completeData)
  {
    Task* task = NEW(Task,run,runData,elts,complete,completeData);
    activeTasks++;
    taskMutex.lock();
    if (numTasks >= MAX_TASKS)
      FATAL("too many tasks generated");
    tasks[numTasks++] = task;
    taskMutex.unlock();
  }

  void TaskScheduler::go() {
    barrier.wait();
    run(0);
    barrier.wait();
  }

  void TaskScheduler::run(size_t tid)
  {
    while (int32(activeTasks))
    {
      Task* task = NULL;
      size_t elt = 0;

      taskMutex.lock();
      if (int32(numTasks)) {
        task = tasks[int32(numTasks)-1];
        elt = --task->started;
        if (elt == 0) numTasks--;
      }
      taskMutex.unlock();
      if (!task) continue;

      /* run the task */
      if (task->run)
        task->run(tid,task->runData,elt);

      /* complete the task */
      if (--task->completed == 0) {
        if (task->complete) task->complete(tid,task->completeData);
        DELETE(task);
        activeTasks--;
      }
    }
  }

  void TaskScheduler::threadFunction(Thread* thread)
  {
    /* get thread ID */
    TaskScheduler* This = thread->scheduler;
    size_t tid = thread->tid;
    DELETE(thread);

    /* flush to zero and no denormals */
    _mm_setcsr(_mm_getcsr() | /*FTZ:*/ (1<<15) | /*DAZ:*/ (1<<6));

    This->barrier.wait();
    while (true) {
      This->barrier.wait();
      if (This->terminateThreads) break;
      This->run(tid);
      This->barrier.wait();
    }
  }

  void TaskScheduler::init(int numThreads)
  {
    if (scheduler != NULL) return;
    if (numThreads < 0) scheduler = NEW(TaskScheduler,getNumberOfLogicalThreads());
    else scheduler = NEW(TaskScheduler,numThreads);
  }

  void TaskScheduler::cleanup()
  {
    if (scheduler) {
      DELETE(scheduler);
      scheduler = NULL;
    }
  }

  TaskScheduler* scheduler;
}


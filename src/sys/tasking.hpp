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

#ifndef __PF_TASKING_HPP__
#define __PF_TASKING_HPP__

#include "sys/ref.hpp"
#include "sys/atomic.hpp"

/*                   *** OVERVIEW OF THE TASKING SYSTEM ***
 *
 * Quick recap of what we have here. Basically, a "tasking system" offers the
 * possibility to schedule and asynchronously run functions in shared memory
 * "system threads". This is basically a thread pool. However, we try to propose
 * more in this API by letting the user:
 * 1 - Define *dependencies* between tasks
 * 2 - Setup priorities for each task ie a higher priority task will be more
 *     likely executed than a lower priority one
 * 3 - Setup affinities for each of them ie a task can be "pinned" on some
 *     specific hardware thread (typically useful when something depends on a
 *     context like an OpenGL context)
 *
 * The core of this tasking system is a "Task". A task represents a function to
 * call (to run) later. Each task can specify dependencies in two ways:
 * 1 - "Start dependencies" specified (see below) by Task::starts. Basically, to
 *     be able to start, a task must have all its start dependencies *ended*
 * 2 - "End dependencies" specified (see below) by Task::ends. In that case, tgo
 *     be able to finish, a task must have all its end dependencies *ended*
 * So, task1->starts(task2) means that task2 cannot start before task1 is ended
 * Also, task3->ends(task4) means that task4 cannot end before task3 is ended
 * Note that each task can only start one task and can only end one task
 *
 * Specifying dependencies in that way allows the user to *dynamically* (ie
 * during the task execution) create a direct acyclic graph of tasks (DAG). One
 * may look at the unit tests to see how it basically works.
 *
 * We also classicaly implement a TaskSet which is a function which can be run
 * n times (concurrently on any number of threads). TaskSet are a particularly
 * efficient way to logically create n tasks in one chunk.
 *
 * Last feature we added is the ability to run *some* task (ie the user cannot
 * decide what it will run) from a running task (ie from the run function of the
 * task). The basic idea here is to overcome typical issues with tasking system:
 * How do you handle asynchronous/non-blocking IO? You may want to reschedule a
 * task if the IO takes too long. But in that case, how can you be sure that the
 * scheduler is not going to run the task you just pushed? Our idea (to assert
 * in later tests) is to offer the ability to run *something* already ready to
 * hide the IO latency from the task itself. At least, you can keep the HW
 * thread busy if you want to.
 *
 *               *** SOME DETAILS ABOUT THE IMPLEMENTATION ***
 *
 * First thing is the comments in tasking.cpp which give some details about
 * the implementation. Roughly the implementation is done around three
 * components:
 *
 * 1 - A fast, distributed, fixed size growing pool to allocate / deallocate the
 *     tasks. OK, to be honest, the fact it is a growing pool is a rather
 *     aggressive approach which clearly should be refined later. Well, actually
 *     it is damn fast. I think the idea to keep speed while reducing memory
 *     footprint may be the implementation of a asynchronous reclaim using the
 *     tasking system itself.
 *
 * 2 - A work-stealing technique for all tasks that do not have any affinity.
 *     Basically, each HW thread in the thread pool has his own queue. Each
 *     thread can push new task *only in its own queue*. When a thread tries to
 *     find a ready task, it first tries to pick one from its queue in depth
 *     first order. If its queue is empty, it tries to *steal* a task from
 *     another HW thread in breadth first order. This approach strongly limits
 *     the memory requirement (ie the number of task currently allocated in the
 *     system) while also limiting the contention in the queues. Right now, the
 *     queues still use spin locks but a classical ABP lock free queue is
 *     clearly coming :-)
 *
 * 3 - A classical FIFO queue approach. Beside its work stealing queue, each
 *     thread owns another FIFO dedicated to tasks with affinities. Basically,
 *     this is more or less the opposite of work stealing: instead of pushing a
 *     affinity task in its own queue, the thread just puts it in the queue
 *     associated to the affinity
 *
 * Finally, note that we handle priorities in a somehow approximate way. Since
 * the system is entirely distributed, it is extremely hard to ensure that, when
 * a high priority task is ready somewhere in the system, it is going to be run
 * as soon as possible. Since it is hard, we use an approximate scheduling
 * strategy by *multiplexing* queues:
 * 0 - If the user returns a task to run, we run it regardless anything else in
 *     the system
 * 1 - The scheduler then explores both private queues and tries to pick up the
 *     one with the largest priority across the two queues. If tasks of equal
 *     priority are found in both queues, the affinity queue is preferred
 * 2 - If no task was found in 2, we try to steal a task from a random other
 *     queue. Here also, we take the one with the highest priority
 *
 * Therefore, the priorities are in that order:
 * (">" means "has a higher priority than")
 *
 * User-Returned Task [regardless the priority] >
 * Affinity-Queue::critical Task                >
 * Self-Work-Stealing-Queue::critical Task      >
 * Affinity-Queue::high Task                    >
 * Self-Work-Stealing-Queue::high Task          >
 * Affinity-Queue::normal Task                  >
 * Self-Work-Stealing-Queue::normal Task        >
 * Affinity-Queue::low Task                     >
 * Self-Work-Stealing-Queue::low Task           >
 * Victim-Work-Stealing-Queue::critical Task    >
 * Victim-Work-Stealing-Queue::high Task        >
 * Victim-Work-Stealing-Queue::normal Task      >
 * Victim-Work-Stealing-Queue::low Task         >
 *
 * As a final note, this tasking system is basically a mix of ideas from TBB (of
 * course :-) ), various tasking systems I used on Ps3 and also various tasking
 * system I had the chance to use and experiment when I was at Intel Labs during
 * the LRB era.
 */

/*! Use or not the fast allocator */
#define PF_TASK_USE_DEDICATED_ALLOCATOR 1

/*! Store or not run-time statistics in the tasking system */
#define PF_TASK_STATICTICS 0

/*! Give number of tries before yielding (multiplied by number of threads) */
#define PF_TASK_TRIES_BEFORE_YIELD 256

/*! Maximum time in milliseconds the thread can be swicthed off */
#define PF_TASK_MAX_YIELD_TIME 1024

/*! Main thread (the one that the system gives us) is always 0 */
#define PF_TASK_MAIN_THREAD 0

/*! No affinity means that the task can rn anywhere */
#define PF_TASK_NO_AFFINITY 0xffffu

/*! Maximum time the thread is yielded */
namespace pf {

  /*! A task with a higher priority will be preferred to a task with a lower
   *  priority. Note that the system does not completely comply with
   *  priorities. Basically, because the system is distributed, it is possible
   *  that one particular worker thread processes a low priority task while
   *  another thread actually has higher priority tasks currently available
   */
  struct TaskPriority {
    enum {
      CRITICAL = 0u,
      HIGH     = 1u,
      NORMAL   = 2u,
      LOW      = 3u,
      NUM      = 4u,
      INVALID  = 0xffffu
    };
  };

  /*! Describe the current state of a task. This asserts the correctness of the
   * operations (like Task::starts or Task::ends which only operates on tasks
   * with specific states)
   */
  struct TaskState {
    enum {
      NEW       = 0u,
      SCHEDULED = 1u,
      READY     = 2u,
      RUNNING   = 3u,
      DONE      = 4u,
      NUM       = 5u,
      INVALID   = 0xffffu
    };
  };

  /*! Interface for all tasks handled by the tasking system */
  class Task : public RefCount
  {
  public:
    /*! It can complete one task and can be continued by one other task */
    INLINE Task(const char *taskName = NULL);
    /*! To override while specifying a task. This is basically the code to
     * execute. The user can optionally return a task which will by-pass the
     * scheduler and will run *immediately* after this one. This is a classical
     * continuation passing style strategy when we have a depth first scheduling
     */
    virtual Task* run(void) = 0;
    /*! Task is built and will be ready when all start dependencies are over */
    void scheduled(void);
    /*! The given task cannot *start* as long as "other" is not complete */
    INLINE void starts(Task *other);
    /*! The given task cannot *end* as long as "other" is not complete */
    INLINE void ends(Task *other);
    /*! Set / get task priority and affinity */
    INLINE void setPriority(uint8 prio);
    INLINE void setAffinity(uint16 affi);
    INLINE uint8 getPriority(void) const;
    INLINE uint16 getAffinity(void) const;
    /*! The scheduler will run tasks as long as this task is not done */
    void waitForCompletion(void);

#if PF_TASK_USE_DEDICATED_ALLOCATOR
    /*! Tasks use a scalable fixed size allocator */
    void* operator new(size_t size);
    /*! Deallocations go through the dedicated allocator too */
    void operator delete(void* ptr);
#endif /* PF_TASK_USE_DEDICATED_ALLOCATOR */

  private:
    template <int> friend struct TaskWorkStealingQueue; //!< Contains tasks
    template <int> friend struct TaskAffinityQueue;     //!< Contains tasks
    friend class TaskSet;      //!< Will tweak the ending criterium
    friend class TaskScheduler;//!< Needs to access everything
    Ref<Task> toBeEnded;       //!< Signals it when finishing
    Ref<Task> toBeStarted;     //!< Triggers it when ready
    const char *name;          //!< Debug facility mostly
    Atomic32 toStart;          //!< MBZ before starting
    Atomic32 toEnd;            //!< MBZ before ending
    uint16 affinity;           //!< The task will run on a particular thread
    uint8 priority;            //!< Task priority
    volatile uint8 state;      //!< Assert correctness of the operations
  };

  /*! Allow the run function to be executed several times */
  class TaskSet : public Task
  {
  public:
    /*! elemNum is the number of times to execute the run function */
    INLINE TaskSet(size_t elemNum, const char *name = NULL);
    /*! This function is user-specified */
    virtual void run(size_t elemID) = 0;

  private:
    virtual Task* run(void); //!< Reimplemented for all task sets
    Atomic elemNum;          //!< Number of outstanding elements
  };

  /*! Mandatory before creating and running any task (MAIN THREAD) */
  void TaskingSystemStart(void);

  /*! Make the main thread enter the tasking system (MAIN THREAD) */
  void TaskingSystemEnd(void);

  /*! Make the main thread enter the tasking system (MAIN THREAD) */
  void TaskingSystemEnter(void);

  /*! Signal the *main* thread only to stop (THREAD SAFE) */
  void TaskingSystemInterruptMain(void);

  /*! Signal *all* threads to stop (THREAD SAFE) */
  void TaskingSystemInterrupt(void);

  /*! Number of threads currently in the tasking system (*including main*) */
  uint32 TaskingSystemGetThreadNum(void);

  /*! Return the ID of the calling thread (between 0 and threadNum) */
  uint32 TaskingSystemGetThreadID(void);

  /*! Run any task (in READY state) in the system. Can be used from a task::run
   *  to overlap some IO for example. Return true if anything was executed
   */
  bool TaskingSystemRunAnyTask(void);

  ///////////////////////////////////////////////////////////////////////////
  /// Implementation of the inlined functions
  ///////////////////////////////////////////////////////////////////////////

  INLINE Task::Task(const char *taskName) :
    name(taskName),
    toStart(1), toEnd(1),
    affinity(PF_TASK_NO_AFFINITY),
    priority(uint8(TaskPriority::NORMAL)),
    state(uint8(TaskState::NEW))
  {
    // The scheduler will remove this reference once the task is done
    this->refInc();
  }

  INLINE void Task::starts(Task *other) {
    if (UNLIKELY(other == NULL)) return;
    assert(other->state == TaskState::NEW);
    if (UNLIKELY(this->toBeStarted)) return; // already a task to start
    other->toStart++;
    this->toBeStarted = other;
  }

  INLINE void Task::ends(Task *other) {
    if (UNLIKELY(other == NULL)) return;
#ifndef NDEBUG
    const uint32 state = other->state;
    assert(state == TaskState::NEW ||
           state == TaskState::SCHEDULED ||
           state == TaskState::RUNNING);
#endif /* NDEBUG */
    if (UNLIKELY(this->toBeEnded)) return;  // already a task to end
    other->toEnd++;
    this->toBeEnded = other;
  }

  INLINE void Task::setPriority(uint8 prio) {
    assert(this->state == TaskState::NEW);
    this->priority = prio;
  }

  INLINE void Task::setAffinity(uint16 affi) {
    assert(this->state == TaskState::NEW);
    this->affinity = affi;
  }

  INLINE uint8 Task::getPriority(void)  const { return this->priority; }
  INLINE uint16 Task::getAffinity(void) const { return this->affinity; }

  INLINE TaskSet::TaskSet(size_t elemNum, const char *name) :
    Task(name), elemNum(elemNum) {}

} /* namespace pf */

#endif /* __PF_TASKING_HPP__ */


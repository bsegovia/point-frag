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
#include "mutex.hpp"

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
    TaskMain(const char *name = NULL);
  };

  /*! Chained tasks form a single list. Each task schedules its successor */
  class TaskChained : public Task
  {
  public:
    TaskChained(void);
    virtual Task *run(void);
    INLINE void setNext(Task *succ_) { this->succ = succ_; }
  protected:
    Task *succ;
  };

  /*! Dependency root is the first task that triggers the multiple
   *  dependencies. It includes a mutex protected variable that allows the
   *  start dependencies to be added at any time
   */
  class TaskDependencyRoot : public TaskChained
  {
  public:
    TaskDependencyRoot(void);
    virtual Task *run(void);
    /*! Lock the run function (then we can modify variable "done") */
    void lock(void);
    /*! Unlock the run function */
    void unlock(void);
    /*! Once done, the parent task cannot be a input dependency anymore */
    INLINE bool isDone(void) const { return done; }
  private:
    MutexActive mutex;  //!< Protect isDone variable
    volatile bool done; //!< true means the parent task is ended
  };

  /*! MultipleDependency is a policy that allows a task to start and end
   *  several tasks (instead of one with the standard task).
   *  - More importantly, MultipleDependency also relaxes the requirement to add
   *  a start dependency (method "multiStarts"). multiStarts can be called
   *  from anywhere regardless the current state the task (it can be NEW,
   *  RUNNING or DONE)
   *  - Internally, it simply uses a list of dummy tasks to trigger the
   *  dependencies
   */
  template <typename T>
  class MultiDependencyPolicy
  {
  public:
    /*! Allocate the chained list of dependency tasks */
    INLINE MultiDependencyPolicy(void);
    /*! Add one more task to start */
    INLINE void multiStarts(Task *other);
    /*! Add one more task to end */
    INLINE void multiEnds(Task *other);
  private:
    Ref<TaskDependencyRoot> tail; //!< First dependency task
    TaskChained *head;            //!< Ends us and triggers the dependencies
  };

  /*! Task with multiple dependencies */
  class TaskInOut : public Task, public MultiDependencyPolicy<TaskInOut>
  {
  public:
    INLINE TaskInOut(const char *name = NULL) : Task(name) {}
  };

  /*! Encapsulates functor (and anonymous lambda) */
  template <typename T, typename TaskType = Task>
  class TaskFunctor : public TaskType
  {
  public:
    INLINE TaskFunctor(const T &functor, const char *name = NULL);
    virtual Task *run(void);
  private:
    T functor;
  };

  /*! Spawn a task from a functor */
  template <typename TaskType, typename FunctorType>
  TaskType *spawn(const char *name, const FunctorType &functor) {
    typedef TaskFunctor<FunctorType, TaskType> TaskClass;
    return PF_NEW(TaskClass, functor, name);
  }

  ///////////////////////////////////////////////////////////////////////////
  /// Implementation of methods and functions
  ///////////////////////////////////////////////////////////////////////////

  template <typename T>
  INLINE MultiDependencyPolicy<T>::MultiDependencyPolicy(void)
  {
    this->tail = PF_NEW(TaskDependencyRoot);
    this->head = this->tail;
    static_cast<T*>(this)->starts(head);
    head->scheduled();
  }

  template <typename T>
  INLINE void MultiDependencyPolicy<T>::multiStarts(Task *other)
  {
    if (UNLIKELY(other == NULL)) return;
    if (tail->isDone() == true) return;
    tail->lock();
    if (tail->isDone() == false) {
      TaskChained *newHead = PF_NEW(TaskChained);
      newHead->starts(other);
      head->setNext(newHead);
      head = newHead;
    }
    tail->unlock();
  }

  template <typename T>
  INLINE void MultiDependencyPolicy<T>::multiEnds(Task *other)
  {
    if (UNLIKELY(other == NULL)) return;
#ifndef NDEBUG
    const uint32 state = other->getState();
    PF_ASSERT(state == TaskState::NEW ||
              state == TaskState::SCHEDULED ||
              state == TaskState::RUNNING);
#endif /* NDEBUG */
    TaskChained *newHead = PF_NEW(TaskChained);
    newHead->ends(other);
    head->setNext(newHead);
    head = newHead;
  }

  template <typename T, typename TaskType>
  INLINE TaskFunctor<T, TaskType>::TaskFunctor(const T &functor, const char *name) :
    TaskType(name), functor(functor) {}

  template <typename T, typename TaskType>
  Task *TaskFunctor<T, TaskType>::run(void) { functor(); return NULL; }

} /* namespace pf */

#endif /* __PF_TASKING_UTILITY_HPP__ */

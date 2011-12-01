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

#include "sys/condition.hpp"

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(__GNUC__)

namespace pf
{
  // This is an implementation of POSIX "compatible" condition variables for
  // Win32, as described by Douglas C. Schmidt and Irfan Pyarali:
  // http://www.cs.wustl.edu/~schmidt/win32-cv-1.html. The code is directly
  // readapted from the glfw source code that may be found here:
  // http://www.glfw.org
  enum {
      MINGW32_COND_SIGNAL     = 0,
      MINGW32_COND_BROADCAST  = 1
  };

  /*! Implement the internal condition variable implementation Mingw lacks */
  struct Mingw32Cond
  {
    HANDLE events[2];                   //<! Signal and broadcast event HANDLEs
    unsigned int waiters_count;         //<! Count of the number of waiters
    CRITICAL_SECTION waiters_count_lock;//!< Serialize access to <waiters_count>
  };

  ConditionSys::ConditionSys ()
  {
    cond = (Mingw32Cond *) PF_NEW(Mingw32Cond);
    ((Mingw32Cond *)cond)->waiters_count = 0;
    ((Mingw32Cond *)cond)->events[MINGW32_COND_SIGNAL]    = CreateEvent(NULL, FALSE, FALSE, NULL);
    ((Mingw32Cond *)cond)->events[MINGW32_COND_BROADCAST] = CreateEvent(NULL, TRUE, FALSE, NULL);
    InitializeCriticalSection(&((Mingw32Cond *)cond)->waiters_count_lock);
  }

  ConditionSys::~ConditionSys ()
  {
    CloseHandle(((Mingw32Cond *)cond)->events[MINGW32_COND_SIGNAL]);
    CloseHandle(((Mingw32Cond *)cond)->events[MINGW32_COND_BROADCAST]);
    DeleteCriticalSection(&((Mingw32Cond *)cond)->waiters_count_lock);
    PF_DELETE((Mingw32Cond *)cond);
  }

  void ConditionSys::wait(MutexSys& mutex)
  {
    Mingw32Cond *cv = (Mingw32Cond *) cond;
    int result, last_waiter;
    DWORD timeout_ms;

    // Avoid race conditions
    EnterCriticalSection(&cv->waiters_count_lock);
    cv->waiters_count ++;
    LeaveCriticalSection(&cv->waiters_count_lock);

    // It's ok to release the mutex here since Win32 manual-reset events
    // maintain state when used with SetEvent()
    LeaveCriticalSection((CRITICAL_SECTION *) mutex.mutex);
    timeout_ms = INFINITE;

    // Wait for either event to become signaled due to glfwSignalCond or
    // glfwBroadcastCond being called
    result = WaitForMultipleObjects(2, cv->events, FALSE, timeout_ms);

    // Check if we are the last waiter
    EnterCriticalSection(&cv->waiters_count_lock);
    cv->waiters_count --;
    last_waiter = (result == WAIT_OBJECT_0 + MINGW32_COND_BROADCAST) &&
                  (cv->waiters_count == 0);
    LeaveCriticalSection(&cv->waiters_count_lock);

    // Some thread called glfwBroadcastCond
    if (last_waiter) {
      // We're the last waiter to be notified or to stop waiting, so
      // reset the manual event
      ResetEvent(cv->events[MINGW32_COND_BROADCAST]);
    }

    // Reacquire the mutex
    EnterCriticalSection((CRITICAL_SECTION *) mutex.mutex);
  }

  void ConditionSys::broadcast()
  {
    Mingw32Cond *cv = (Mingw32Cond *) cond;
    int have_waiters;

    // Avoid race conditions
    EnterCriticalSection(&cv->waiters_count_lock);
    have_waiters = cv->waiters_count > 0;
    LeaveCriticalSection(&cv->waiters_count_lock);

    if (have_waiters)
      SetEvent(cv->events[MINGW32_COND_BROADCAST]);
  }
} /* namespace pf */
#else

namespace pf
{
  /*! system condition using windows API */
  ConditionSys::ConditionSys () { cond = PF_NEW(CONDITION_VARIABLE); InitializeConditionVariable((CONDITION_VARIABLE*)cond); }
  ConditionSys::~ConditionSys() { PF_DELETE((CONDITION_VARIABLE*)cond); }
  void ConditionSys::wait(MutexSys& mutex) { SleepConditionVariableCS((CONDITION_VARIABLE*)cond, (CRITICAL_SECTION*)mutex.mutex, INFINITE); }
  void ConditionSys::broadcast() { WakeAllConditionVariable((CONDITION_VARIABLE*)cond); }
} /* namespace pf */
#endif /* __GNUC__ */
#endif /* __WIN32__ */

#if defined(__UNIX__)
#include <pthread.h>
namespace pf
{
  ConditionSys::ConditionSys () { cond = PF_NEW(pthread_cond_t); pthread_cond_init((pthread_cond_t*)cond,NULL); }
  ConditionSys::~ConditionSys() { PF_DELETE((pthread_cond_t*)cond); }
  void ConditionSys::wait(MutexSys& mutex) { pthread_cond_wait((pthread_cond_t*)cond, (pthread_mutex_t*)mutex.mutex); }
  void ConditionSys::broadcast() { pthread_cond_broadcast((pthread_cond_t*)cond); }
} /* namespace pf */
#endif /* __UNIX__ */


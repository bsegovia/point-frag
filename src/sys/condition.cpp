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

namespace pf
{
  /*! system condition using windows API */
  ConditionSys::ConditionSys () { cond = NEW(CONDITION_VARIABLE); InitializeConditionVariable((CONDITION_VARIABLE*)cond); }
  ConditionSys::~ConditionSys() { DELETE((CONDITION_VARIABLE*)cond); }
  void ConditionSys::wait( MutexSys& mutex ) { SleepConditionVariableCS((CONDITION_VARIABLE*)cond, (CRITICAL_SECTION*)mutex.mutex, INFINITE); }
  void ConditionSys::broadcast() { WakeAllConditionVariable((CONDITION_VARIABLE*)cond); }
}
#endif

#if defined(__UNIX__)
#include <pthread.h>
namespace pf
{
  ConditionSys::ConditionSys () { cond = NEW(pthread_cond_t); pthread_cond_init((pthread_cond_t*)cond,NULL); }
  ConditionSys::~ConditionSys() { DELETE((pthread_cond_t*)cond); }
  void ConditionSys::wait(MutexSys& mutex) { pthread_cond_wait((pthread_cond_t*)cond, (pthread_mutex_t*)mutex.mutex); }
  void ConditionSys::broadcast() { pthread_cond_broadcast((pthread_cond_t*)cond); }
}
#endif


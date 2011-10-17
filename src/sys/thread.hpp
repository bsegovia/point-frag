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

#ifndef __PF_THREAD_HPP__
#define __PF_THREAD_HPP__

#include "sys/platform.hpp"

namespace pf
{
  /*! Type for thread */
  typedef struct opaque_thread_t* thread_t;

  /*! Signature of thread start function */
  typedef void (*thread_func)(void*);

  /*! Creates a hardware thread running on specific logical thread */
  thread_t createThread(thread_func f, void* arg, size_t stack_size = 0, int affinity = -1);

  /*! Set affinity of the calling thread */
  void setAffinity(int affinity);

  /*! The thread calling this function gets yielded for a number of seconds */
  void yield(int time = 0);

  /*! Waits until the given thread has terminated */
  void join(thread_t tid);

  /*! Destroy handle of a thread */
  void destroyThread(thread_t tid);

  /*! Type for handle to thread local storage */
  typedef struct opaque_tls_t* tls_t;

  /*! Creates thread local storage */
  tls_t createTls();

  /*! Set the thread local storage pointer */
  void setTls(tls_t tls, void* const ptr);

  /*! Return the thread local storage pointer */
  void* getTls(tls_t tls);

  /*! Destroys thread local storage identifier */
  void destroyTls(tls_t tls);
}

#endif /* __PF_THREAD_HPP__ */


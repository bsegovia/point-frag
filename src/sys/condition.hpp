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

#ifndef __PF_CONDITION_H__
#define __PF_CONDITION_H__

#include "sys/mutex.hpp"

namespace pf
{
  class ConditionSys
  {
  public:
    ConditionSys( void );
    ~ConditionSys( void );
    void wait( class MutexSys& mutex );
    void broadcast( void );

  protected:
    void* cond;
  };
}

#endif


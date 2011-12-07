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

namespace pf
{
  /*! Make the main thread return to the top-level function */
  class TaskInterruptMain : public Task
  {
  public:
    TaskInterruptMain(void) : Task("TaskInterruptMain") {}
    virtual Task *run(void) {
      TaskingSystemInterruptMain();
      return NULL;
    }
  };

  /*! Does nothing but can be used for synchronization */
  class TaskDummy : public Task
  {
  public:
    TaskDummy(void) : Task("TaskDummy") {}
    virtual Task *run(void) { return NULL; }
  };

} /* namespace pf */

#endif /* __PF_TASKING_UTILITY_HPP__ */


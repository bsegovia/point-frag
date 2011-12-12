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

#include "tasking_utility.hpp"

namespace pf
{
  TaskInterruptMain::TaskInterruptMain(void) : Task("TaskInterruptMain") {}
  Task *TaskInterruptMain::run(void) {
    TaskingSystemInterruptMain();
    return NULL;
  }

  TaskDummy::TaskDummy(void) : Task("TaskDummy") {}
  Task *TaskDummy::run(void) { return NULL; }

  TaskChained::TaskChained(void) : Task("TaskChained"), succ(NULL) {}
  Task *TaskChained::run(void) { return succ; }

  TaskDependencyRoot::TaskDependencyRoot(void) : done(false) {}
  Task *TaskDependencyRoot::run(void) {
    this->lock();
    done = true;
    this->unlock();
    return succ;
  }
  void TaskDependencyRoot::lock(void)   { mutex.lock(); }
  void TaskDependencyRoot::unlock(void) { mutex.unlock(); }

  TaskMain::TaskMain(const char *name) : Task(name) {
    this->setAffinity(PF_TASK_MAIN_THREAD);
  }

} /* namespace pf */


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

#include "sys/tasking.hpp"
#include "sys/ref.hpp"
#include "sys/thread.hpp"
#include "sys/mutex.hpp"
#include "sys/sysinfo.hpp"

#include "utest/utest.hpp"

#define START_UTEST(TEST_NAME)                          \
void TEST_NAME(void)                                    \
{                                                       \
  std::cout << std::endl << "starting " <<              \
              #TEST_NAME << std::endl;

#define END_UTEST(TEST_NAME)                            \
  std::cout << "ending " << #TEST_NAME << std::endl;    \
}

using namespace pf;

///////////////////////////////////////////////////////////////////////////////
// Very simple test which basically does nothing
///////////////////////////////////////////////////////////////////////////////
class NothingTask : public Task {
public:
  virtual Task* run(void) { return NULL; }
};

class DoneTask : public Task {
public:
  virtual Task* run(void) { TaskingSystemInterruptMain(); return NULL; }
};

START_UTEST(TestDummy)
  Task *done = PF_NEW(DoneTask);
  Task *nothing = PF_NEW(NothingTask);
  nothing->starts(done);
  done->scheduled();
  nothing->scheduled();
  TaskingSystemEnter();
END_UTEST(TestDummy)

///////////////////////////////////////////////////////////////////////////////
// Simplest taskset test. An array is filled by each worker
///////////////////////////////////////////////////////////////////////////////
class SimpleTaskSet : public TaskSet {
public:
  INLINE SimpleTaskSet(size_t elemNum, uint32 *array_) :
    TaskSet(elemNum), array(array_) {}
  virtual void run(size_t elemID) { array[elemID] = 1u; }
  uint32 *array;
};

START_UTEST(TestTaskSet)
  const size_t elemNum = 1 << 20;
  uint32 *array = PF_NEW_ARRAY(uint32, elemNum);
  for (size_t i = 0; i < elemNum; ++i) array[i] = 0;
  double t = getSeconds();
  Task *done = PF_NEW(DoneTask);
  Task *taskSet = PF_NEW(SimpleTaskSet, elemNum, array);
  taskSet->starts(done);
  done->scheduled();
  taskSet->scheduled();
  TaskingSystemEnter();
  t = getSeconds() - t;
  std::cout << t * 1000. << " ms" << std::endl;
  for (size_t i = 0; i < elemNum; ++i)
    FATAL_IF(array[i] == 0, "TestTaskSet failed");
  PF_DELETE_ARRAY(array);
END_UTEST(TestTaskSet)

///////////////////////////////////////////////////////////////////////////////
// We create a binary tree of tasks here. Each task spawn a two children upto a
// given maximum level. Then, a atomic value is updated per leaf. In that test,
// all tasks complete the ROOT directly
///////////////////////////////////////////////////////////////////////////////
enum { maxLevel = 20u };

/*! One node task per node in the tree. Task completes the root */
class NodeTask : public Task {
public:
  INLINE NodeTask(Atomic &value_, uint32 lvl_, Task *root_=NULL) :
    value(value_), lvl(lvl_) {
    this->root = root_ == NULL ? this : root_;
  }
  virtual Task* run(void);
  Atomic &value;
  Task *root;
  uint32 lvl;
};

Task* NodeTask::run(void) {
  if (this->lvl == maxLevel)
    this->value++;
  else {
    Task *left  = PF_NEW(NodeTask, this->value, this->lvl+1, this->root);
    Task *right = PF_NEW(NodeTask, this->value, this->lvl+1, this->root);
    left->ends(this->root);
    right->ends(this->root);
    left->scheduled();
    right->scheduled();
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Same as NodeTask but we use a continuation passing style strategy to improve
// the system throughtput
///////////////////////////////////////////////////////////////////////////////

/*! One node task per node in the tree. Task completes the root */
class NodeTaskOpt : public Task {
public:
  INLINE NodeTaskOpt(Atomic &value_, uint32 lvl_, Task *root_=NULL) :
    value(value_), lvl(lvl_) {
    this->root = root_ == NULL ? this : root_;
  }
  virtual Task* run(void);
  Atomic &value;
  Task *root;
  uint32 lvl;
};

Task* NodeTaskOpt::run(void) {
  if (this->lvl == maxLevel) {
    this->value++;
    return NULL;
  } else {
    Task *left  = PF_NEW(NodeTask, this->value, this->lvl+1, this->root);
    Task *right = PF_NEW(NodeTask, this->value, this->lvl+1, this->root);
    left->ends(this->root);
    right->ends(this->root);
    left->scheduled();
    return right;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Same as NodeTask but here each task completes its parent task directly. This
// stresses the completion system but strongly limits cache line contention
///////////////////////////////////////////////////////////////////////////////

/*! One node task per node in the tree. Task completes its parent */
class CascadeNodeTask : public Task {
public:
  INLINE CascadeNodeTask(Atomic &value_, uint32 lvl_, Task *root_=NULL) :
    value(value_), lvl(lvl_) {}
  virtual Task* run(void);
  Atomic &value;
  uint32 lvl;
};

Task *CascadeNodeTask::run(void) {
  if (this->lvl == maxLevel)
    this->value++;
  else {
    Task *left  = PF_NEW(CascadeNodeTask, this->value, this->lvl+1);
    Task *right = PF_NEW(CascadeNodeTask, this->value, this->lvl+1);
    left->ends(this);
    right->ends(this);
    left->scheduled();
    right->scheduled();
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Same as CascadeNodeTask but with continuation passing style tasks
///////////////////////////////////////////////////////////////////////////////
class CascadeNodeTaskOpt : public Task {
public:
  INLINE CascadeNodeTaskOpt(Atomic &value_, uint32 lvl_, Task *root_=NULL) :
    value(value_), lvl(lvl_) {}
  virtual Task* run(void);
  Atomic &value;
  uint32 lvl;
};

Task *CascadeNodeTaskOpt::run(void) {
  if (this->lvl == maxLevel) {
    this->value++;
    return NULL;
  } else {
    Task *left  = PF_NEW(CascadeNodeTask, this->value, this->lvl+1);
    Task *right = PF_NEW(CascadeNodeTask, this->value, this->lvl+1);
    left->ends(this);
    right->ends(this);
    left->scheduled();
    return right;
  }
}

/*! For all tree tests */
template<typename NodeType>
START_UTEST(TestTree)
  Atomic value(0u);
  std::cout << "nodeNum = " << (2 << maxLevel) - 1 << std::endl;
  double t = getSeconds();
  Task *done = PF_NEW(DoneTask);
  Task *root = PF_NEW(NodeType, value, 0);
  root->starts(done);
  done->scheduled();
  root->scheduled();
  TaskingSystemEnter();
  t = getSeconds() - t;
  std::cout << t * 1000. << " ms" << std::endl;
  FATAL_IF(value != (1 << maxLevel), "TestTree failed");
END_UTEST(TestTree)

///////////////////////////////////////////////////////////////////////////////
// We try to stress the internal allocator here
///////////////////////////////////////////////////////////////////////////////
class AllocateTask : public TaskSet {
public:
  AllocateTask(size_t elemNum) : TaskSet(elemNum) {}
  virtual void run(size_t elemID);
  enum { allocNum = 1 << 10 };
  enum { iterNum = 1 << 5 };
};

void AllocateTask::run(size_t elemID) {
  Task *tasks[allocNum];
  for (int j = 0; j < iterNum; ++j) {
    const int taskNum = rand() % allocNum;
    for (int i = 0; i < taskNum; ++i) tasks[i] = PF_NEW(NothingTask);
    for (int i = 0; i < taskNum; ++i) PF_DELETE(tasks[i]);
  }
}

START_UTEST(TestAllocator)
  Task *done = PF_NEW(DoneTask);
  Task *allocate = PF_NEW(AllocateTask, 1 << 10);
  double t = getSeconds();
  allocate->starts(done);
  done->scheduled();
  allocate->scheduled();
  TaskingSystemEnter();
  t = getSeconds() - t;
  std::cout << t * 1000. << " ms" << std::endl;
END_UTEST(TestAllocator)

///////////////////////////////////////////////////////////////////////////////
// We are making the queue full to make the system recurse to empty it
///////////////////////////////////////////////////////////////////////////////
class FullTask : public Task {
public:
  enum { taskToSpawn = 1u << 16u };
  FullTask(const char *name, Atomic &counter, int lvl = 0) :
    Task(name), counter(counter), lvl(lvl) {}
  virtual Task* run(void) {
    if (lvl == 0)
      for (size_t i = 0; i < taskToSpawn; ++i) {
        Task *task = PF_NEW(FullTask, "FullTaskLvl1", counter, 1);
        task->ends(this);
        task->scheduled();
      }
    else
      counter++;
    return NULL;
  }
  Atomic &counter;
  int lvl;
};

START_UTEST(TestFullQueue)
  Atomic counter(0u);
  double t = getSeconds();
  Task *done = PF_NEW(DoneTask);
  for (size_t i = 0; i < 64; ++i) {
    Task *task = PF_NEW(FullTask, "FullTask", counter);
    task->starts(done);
    task->scheduled();
  }
  done->scheduled();
  TaskingSystemEnter();
  t = getSeconds() - t;
  std::cout << t * 1000. << " ms" << std::endl;
  FATAL_IF (counter != 64 * FullTask::taskToSpawn, "TestFullQueue failed");
END_UTEST(TestFullQueue)

///////////////////////////////////////////////////////////////////////////////
// We spawn a lot of affinity jobs to saturate the affinity queues
///////////////////////////////////////////////////////////////////////////////
class AffinityTask : public Task {
public:
  enum { taskToSpawn = 2048u };
  AffinityTask(Task *done, Atomic &counter, int lvl = 0) :
    Task("AffinityTask"), counter(counter), lvl(lvl) {}
  virtual Task *run(void) {
    if (lvl == 1)
      counter++;
    else {
      const uint32 threadNum = TaskingSystemGetThreadNum();
      for (uint32 i = 0; i < taskToSpawn; ++i) {
        Task *task = PF_NEW(AffinityTask, done.ptr, counter, 1);
        task->setAffinity(i % threadNum);
        task->ends(this);
        task->scheduled();
      }
    }
    return NULL;
  }
  Atomic &counter;
  Ref<Task> done;
  int lvl;
};

START_UTEST(TestAffinity)
  enum { batchNum = 512 };
  for (int i = 0; i < 8; ++i) {
    Atomic counter(0u);
    double t = getSeconds();
    Ref<Task> done = PF_NEW(DoneTask);
    for (size_t i = 0; i < batchNum; ++i) {
      Task *task = PF_NEW(AffinityTask, done.ptr, counter);
      task->starts(done.ptr);
      task->scheduled();
    }
    done->scheduled();
    TaskingSystemEnter();
    t = getSeconds() - t;
    std::cout << t * 1000. << " ms" << std::endl;
    std::cout << counter << std::endl;
    FATAL_IF (counter != batchNum * AffinityTask::taskToSpawn, "TestAffinity failed");
  }
END_UTEST(TestAffinity)

///////////////////////////////////////////////////////////////////////////////
// Exponential Fibonnaci to stress the task spawning and the completions
///////////////////////////////////////////////////////////////////////////////
static Atomic fiboNum(0u);
class FiboSpawnTask : public Task {
public:
  FiboSpawnTask(uint64 rank, uint64 *root = NULL) :
    Task("FiboSpawnTask"), rank(rank), root(root) {fiboNum++;}
  virtual Task* run(void);
  uint64 rank, sumLeft, sumRight;
  uint64 *root;
};

class FiboSumTask : public Task {
public:
  FiboSumTask(FiboSpawnTask *fibo) : Task("FiboSumTask"), fibo(fibo) {}
  virtual Task* run(void);
  FiboSpawnTask *fibo;
};

Task *FiboSpawnTask::run(void) {
  if (rank > 1) {
    FiboSpawnTask *left = PF_NEW(FiboSpawnTask, rank-1, &this->sumLeft);
    FiboSpawnTask *right = PF_NEW(FiboSpawnTask, rank-2, &this->sumRight);
    FiboSumTask *sum = PF_NEW(FiboSumTask, this);
    left->starts(sum);
    right->starts(sum);
    sum->ends(this);
    sum->scheduled();
    left->scheduled();
    return right;
  } else if (rank == 1) {
    if (root) *root = 1;
    return NULL;
  } else {
    if (root) *root = 0;
    return NULL;
  }
}

Task *FiboSumTask::run(void) {
  assert(fibo);
  if (fibo->root) *fibo->root = fibo->sumLeft + fibo->sumRight;
  return NULL;
}

static uint64 fiboLinear(uint64 rank)
{
  uint64 rn0 = 0, rn1 = 1;
  if (rank == 0) return rn0;
  if (rank == 1) return rn1;
  for (uint64 i = 2; i <= rank; ++i) {
    uint64 sum = rn0 + rn1;
    rn0 = rn1;
    rn1 = sum;
  }
  return rn1;
}

START_UTEST(TestFibo)
  {
    const uint64 rank = rand() % 32;
    uint64 sum;
    double t = getSeconds();
    fiboNum = 0u;
    Ref<FiboSpawnTask> fibo = PF_NEW(FiboSpawnTask, rank, &sum);
    Task *done = PF_NEW(DoneTask);
    fibo->starts(done);
    fibo->scheduled();
    done->scheduled();
    TaskingSystemEnter();
    t = getSeconds() - t;
    std::cout << t * 1000. << " ms" << std::endl;
    std::cout << "Fibonacci Task Num: "<< fiboNum << std::endl;
    FATAL_IF (sum != fiboLinear(rank), "TestFibonacci failed");
  }
END_UTEST(TestFibo)

void utest_tasking(void)
{
  TestDummy();
  TestTree<NodeTaskOpt>();
  TestTree<NodeTask>();
  TestTree<CascadeNodeTaskOpt>();
  TestTree<CascadeNodeTask>();
  TestTaskSet();
  TestAllocator();
  TestFullQueue();
  TestAffinity();
  TestFibo();
}

UTEST_REGISTER(utest_tasking);


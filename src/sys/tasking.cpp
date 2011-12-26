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
#include "sys/condition.hpp"
#include "sys/sysinfo.hpp"

#include <vector>
#include <cstdlib>
#include <emmintrin.h>
#include <stdint.h>

// One important remark about reference counting. Tasks are referenced
// counted but we do not use Ref<Task> here. This is for performance reasons.
// We therefore *manually* handle the extra references the scheduler may have
// on a task. This is nasty but maintains a reasonnable speed in the system.
// Using Ref<Task> in the queues makes the system roughly twice slower

// Convenient shortcut macro for task statistics
#if PF_TASK_STATICTICS
#define IF_TASK_STATISTICS(EXPR) EXPR
#else
#define IF_TASK_STATISTICS(EXPR)
#endif /* PF_TASK_STATISTICS */

// Convenient shortcut macro for task profiling
#if PF_TASK_PROFILER
#define TASK_PROFILE(PROFILER, FN, ...) do {  \
  if (PROFILER) PROFILER->FN(__VA_ARGS__);    \
} while (0)
#else
#define TASK_PROFILE(PROFILER, FN, ...) do {} while(0)
#endif /* PF_TASK_PROFILER */

namespace pf
{
  ///////////////////////////////////////////////////////////////////////////
  /// Declaration of the internal classes of the tasking system
  ///////////////////////////////////////////////////////////////////////////
  class Task;          // Basically an asynchronous function with dependencies
  class TaskSet;       // Idem but can be run N times
  class TaskAllocator; // Dedicated to allocate tasks and task sets
  class TaskScheduler; // Owns the complete system

  /*! Structure used to issue ready-to-process tasks */
  template <int elemNum>
  struct CACHE_LINE_ALIGNED TaskQueue
  {
  public:
    INLINE TaskQueue(void) {
      for (uint32 i = 0; i < TaskPriority::NUM; ++i) head[i] = tail[i] = 0;
    }

    /*! Return the bit mask of the four queues:
     *  - 1 if there is any task
     *  - 0 if empty
     *  Since we properly sort priorities from 0 to 3, using bit scan forward
     *  will return the first non-empty queue with the highest priority
     */
    INLINE int getActiveMask(void) const {
#if defined(__MSVC__)
      // Unfortunately, VS does not support volatile __m128 variables
      PF_COMPILER_READ_WRITE_BARRIER;
      __m128i t, h;
      t.m128i_i64[0] = tail.v.m128i_i64[0];
      h.m128i_i64[0] = head.v.m128i_i64[0];
      t.m128i_i64[1] = tail.v.m128i_i64[1];
      h.m128i_i64[1] = head.v.m128i_i64[1];
      PF_COMPILER_READ_WRITE_BARRIER;
#else
      const __m128i t = __load_acquire(&tail.v);
      const __m128i h = __load_acquire(&head.v);
#endif /* defined(__MSVC__) */
      const __m128i len = _mm_sub_epi32(t, h);
      return _mm_movemask_ps(_mm_castsi128_ps(len));
    }

  protected:
    Task * volatile tasks[TaskPriority::NUM][elemNum]; //!< All tasks currently stored
    typedef MutexActive MutexType;                     //!< Not lock-free right now
    MutexType mutex;
    union {
      INLINE volatile int32& operator[] (int32 prio) { return x[prio]; }
      volatile int32 x[TaskPriority::NUM];
      volatile __m128i v;
    } head, tail;
    PF_ALIGNED_CLASS(CACHE_LINE);
  };

  /*! For work stealing:
   *  - only the owner (ie the victim) inserts tasks
   *  - the owner picks up tasks in depth first order (LIFO)
   *  - the stealers pick up tasks in breadth first order (FIFO)
   */
  template <int elemNum>
  struct TaskWorkStealingQueue : TaskQueue<elemNum> {
    TaskWorkStealingQueue(void)
#if PF_TASK_STATICTICS
      : statInsertNum(0), statGetNum(0), statStealNum(0)
#endif /* PF_TASK_STATICTICS */
    {}

    /*! No need to lock here since only the owner can push a task */
    bool insert(Task &task);
    /*! Both stealers and the victim can pick up a task: we lock */
    Task* get(void);
    /*! Idem: we lock */
    Task* steal(void);

#if PF_TASK_STATICTICS
    void printStats(void) {
      std::cout << "insertNum " << statInsertNum <<
                   ", getNum " << statGetNum <<
                   ", stealNum " << statStealNum << std::endl;
    }
    Atomic32 statInsertNum, statGetNum, statStealNum;
#endif /* PF_TASK_STATICTICS */
  };

  /*! Tasks with affinity go here. For this queue:
   *  - any thread can push a task
   *  - only the owner can pick up tasks
   */
  template <int elemNum>
  struct TaskAffinityQueue : TaskQueue<elemNum> {
    TaskAffinityQueue (void)
#if PF_TASK_STATICTICS
      : statInsertNum(0), statGetNum(0)
#endif /* PF_TASK_STATICTICS */
    {}

    /*! All threads can insert a task. We need to lock */
    bool insert(Task &task);
    /*! Only the owner can pick up tasks. No need to lock */
    Task* get(void);

#if PF_TASK_STATICTICS
    void printStats(void) {
      std::cout << "insertNum " << statInsertNum <<
                   ", getNum " << statGetNum << std::endl;
    }
    Atomic32 statInsertNum, statGetNum;
#endif /* PF_TASK_STATICTICS */
  };

  /*! We will switch off the thread if nothing can be run */
  enum TaskThreadState {
    TASK_THREAD_STATE_SLEEPING = 0,
    TASK_THREAD_STATE_RUNNING  = 1,
    TASK_THREAD_STATE_DEAD     = 2,
    TASK_THREAD_STATE_OUTSIDE  = 3,
    TASK_THREAD_STATE_INVALID  = 0xffffffff
  };

  /*! Per thread state required to run the tasking system */
  class CACHE_LINE_ALIGNED TaskThread
  {
  public:
    TaskThread(void);
    ~TaskThread(void);
    /*! Tell the thread it has to return */
    INLINE void die(void);
    /*! Resume the thread execution. In the case that we have to steal a task
     *  from somewhere, we also provide the threadID that wakes up. Therefore,
     *  we know where to get the task to steal. If the ID is negative, we do not
     *  change the current victim to steal from.
     */
    void wakeUp(int32 threadThatWakesMeUp = -1);
    /*! Check without locking the state before waking up the threads */
    void tryWakeUp(int32 threadThatWakesMeUp = -1);
    /*! Yield the thread using a condition variable */
    void sleep(void);
    enum { queueSize = 512 };                //!< Number of task per queue
    TaskWorkStealingQueue<queueSize> wsQueue;//!< Per thread work stealing queue
    TaskAffinityQueue<queueSize> afQueue;    //!< Per thread affinity queue
    thread_t thread;                //!< System thread handle
    TaskScheduler *scheduler;       //!< It owns us
    ConditionSys cond;              //!< Condition variable for state
    MutexSys mutex;                 //!< Protects condition variable
    volatile TaskThreadState state; //!< SLEEPING or RUNNING?
    size_t threadID;                //!< Our ID in the tasking system
    uint32 victim;                  //!< Next thread to steal from
    uint32 toWakeUp;                //!< Next guy to wake up
#if PF_TASK_STATICTICS
    Atomic sleepNum;
#endif /* PF_TASK_STATICTICS */
    PF_ALIGNED_CLASS(CACHE_LINE);
  };

  /*! Handle the scheduling of all tasks. We first implement a
   *  work-stealing algorithm. Each thread has each own queue where it picks
   *  up task in depth first order. Each thread can also steal other tasks
   *  in breadth first order when his own queue is empty. Each thread also
   *  maintains a FIFO queue that contains jobs that it is the only to run
   *  (this is called "affinity" queue)s
   */
  class CACHE_LINE_ALIGNED TaskScheduler
  {
  public:
    /*! If threadNum == 0, use the maximum number of threads */
    TaskScheduler(int threadNum_ = -1);
    ~TaskScheduler(void);
    /*! Call by the main thread to enter the tasking system */
    void go(void);
    /*! Interrupt all threads */
    INLINE void stopAll(void) {
      for (uint32 i = 0; i < this->queueNum; ++i)
        this->taskThread[i].die();
    }
    /*! Interrupt main thread only */
    INLINE void stopMain(void) { this->taskThread[PF_TASK_MAIN_THREAD].die(); }
#if PF_TASK_PROFILER
    /*! Set the profiler (if activated) */
    INLINE void setProfiler(TaskProfiler *profiler_) {
      this->profiler = profiler_;
    }
#endif /* PF_TASK_PROFILER */
    /*! Number of threads running in the scheduler (not including main) */
    INLINE uint32 getWorkerNum(void) { return uint32(this->workerNum); }
    /*! ID of the calling thread in the tasking system */
    INLINE uint32 getThreadID(void) { return uint32(this->threadID); }
    /*! Try to get a task from all the current queues */
    INLINE Task* getTask(void);
    /*! Run the task and recursively handle the tasks to start and to end */
    void runTask(Task *task);
    /*! Lock the scheduler. The locking thread is the only to run */
    void lock(void);
    /*! Unlock the scheduler */
    void unlock(void);
    /*! Wait the task completion */
    void wait(Ref<Task> task);
    /*! Wait until all queues are empty */
    void waitAll(void);
    /*! Data provided to each thread */
    struct ThreadStartup {
      ThreadStartup(size_t tid, TaskScheduler &scheduler_) :
        tid(tid), scheduler(scheduler_) {}
      size_t tid;
      TaskScheduler &scheduler;
    };

  private:
    /*! Function run by each thread */
    static void threadFunction(ThreadStartup *thread);
    /*! Schedule a task which is now ready to execute */
    INLINE void schedule(Task &task);
    /*! Try to push a task in the queue. Returns false if queues are full */
    INLINE bool trySchedule(Task &task);
    friend class Task;            //!< Tasks ...
    friend class TaskSet;         // ... task sets ...
    friend class TaskAllocator;   // ... task allocator use the tasking system
    friend class TaskThread;      //!< Update the sleeping bitfield
    static THREAD uint32 threadID;//!< ThreadID for each thread
    TaskThread *taskThread;       //!< Per thread state
#if PF_TASK_PROFILER
    TaskProfiler * volatile profiler; //!< Registers events
#endif /* PF_TASK_PROFILER */
    size_t workerNum;             //!< Total number of threads running
    size_t queueNum;              //!< Number of queues (should be workerNum+1)
    volatile size_t sleeping;     //!< Bitfields that gives the sleeping threads
    volatile size_t sleepingNum;  //!< Number of threads sleeping
    MutexActive sleepMutex;       //!< Protect the sleeping field
    CACHE_LINE_ALIGNED volatile int32 locked; //!< To globally lock the tasking system
    PF_ALIGNED_CLASS(CACHE_LINE);
  };

  /*! Allocator per thread */
  class CACHE_LINE_ALIGNED TaskStorage
  {
  public:
    TaskStorage(void) :
#if PF_TASK_STATICTICS
      statNewChunkNum(0), statPushGlobalNum(0), statPopGlobalNum(0),
      statAllocateNum(0), statDeallocateNum(0),
#endif /* PF_TASK_STATICTICS */
      allocator(NULL), allocateNum(0)
    {
      for (size_t i = 0; i < maxHeap; ++i) {
        this->chunk[i] = NULL;
        this->currSize[i] = 0u;
      }
    }
    ~TaskStorage(void) {
      for (size_t i = 0; i < toFree.size(); ++i) PF_ALIGNED_FREE(toFree[i]);
    }

    /*! Will try to allocate from the local storage. Use std::malloc to
     *  allocate a new local chunk
     */
    INLINE void *allocate(size_t sz);
    /*! Free a task and put it in a free list. If too many tasks are
     *  deallocated, return a piece of it to the global heap
     */
    INLINE void deallocate(void *ptr);
    /*! Create a free list and store chunk information */
    void newChunk(uint32 chunkID);
    /*! Push back a group of tasks in the global heap */
    void pushGlobal(uint32 chunkID);
    /*! Pop a group of tasks from the global heap (if none, return NULL) */
    void popGlobal(uint32 chunkID);

#if PF_TASK_STATICTICS
    void printStats(void) {
      std::cout << "newChunkNum " << statNewChunkNum <<
                   ", pushGlobalNum  " << statPushGlobalNum <<
                   ", popGlobalNum  " << statPushGlobalNum <<
                   ", allocateNum  " << statAllocateNum <<
                   ", deallocateNum  " << statDeallocateNum << std::endl;
    }
    Atomic statNewChunkNum, statPushGlobalNum, statPopGlobalNum;
    Atomic statAllocateNum, statDeallocateNum;
#endif /* PF_TASK_STATICTICS */

  private:
    friend class TaskAllocator;
    enum { logChunkSize = 12 };           //!< log2(4KB)
    enum { chunkSize = 1<<logChunkSize }; //!< 4KB when taking memory from std
    enum { maxHeap = 10u };      //!< One heap per size (only power of 2)
    TaskAllocator *allocator;    //!< Handles global heap
    std::vector<void*> toFree;   //!< All chunks allocated (per thread)
    void *chunk[maxHeap];        //!< One heap per size
    uint32 currSize[maxHeap];    //!< Sum of the free task sizes
    int64 allocateNum;           //!< Signed because we can free a task
                                 //   that was allocated elsewhere
  };

  /*! TaskAllocator will speed up task allocation with fast dedicated thread
   *  local storage and fixed size allocation strategy. Each thread maintains
   *  its own list of free tasks. When empty, it first tries to get some tasks
   *  from the global task heap. If the global heap is empty, it just allocates
   *  a new pool of task with a std::malloc. If the local pool is "full", a
   *  chunk of tasks is pushed back into the global heap. Note that the task
   *  allocator is really a growing pool. We *never* give back the chunk of
   *  memory taken from std::malloc (except when the allocator is destroyed)
   */
  class TaskAllocator
  {
  public:
    /*! Constructor. Here this is the total number of threads using the pool (ie
     *  number of worker threads + main thread)
     */
    TaskAllocator(uint32 threadNum);
    ~TaskAllocator(void);
    void *allocate(size_t sz);
    void deallocate(void *ptr);
    enum { maxHeap = TaskStorage::maxHeap };
    enum { maxSize = 1 << maxHeap };
    TaskStorage *local;    //!< Local heaps (per thread and per size)
    void *global[maxHeap]; //!< Global heap shared by all threads
    MutexActive mutex;     //!< To protect the global heap
    uint32 threadNum;      //!< One thread storage per thread
  };

  ///////////////////////////////////////////////////////////////////////////
  /// Implementation of the internal classes of the tasking system
  ///////////////////////////////////////////////////////////////////////////

  // Insertion is only done by the owner of the queues. So, the owner is the
  // only one that modifies the head (since this is the only one that inserts).
  // With proper store_releases, we therefore do not need any lock
  template<int elemNum>
  bool TaskWorkStealingQueue<elemNum>::insert(Task &task) {
    const uint32 prio = task.getPriority();
    if (UNLIKELY(this->head[prio] - this->tail[prio] == elemNum))
      return false;
    __store_release(&task.state, uint8(TaskState::READY));
    __store_release(&this->tasks[prio][this->head[prio] % elemNum], &task);
    const int32 nextHead = this->head[prio] + 1;
    __store_release(&this->head[prio], nextHead);
    IF_TASK_STATISTICS(statInsertNum++);
    return true;
  }

  // get is competing with steal: we use a lock
  template<int elemNum>
  Task* TaskWorkStealingQueue<elemNum>::get(void) {
    if (this->getActiveMask() == 0) return NULL;
    Lock<typename TaskQueue<elemNum>::MutexType> lock(this->mutex);
    const int mask = this->getActiveMask();
    if (mask == 0) return NULL;
    const uint32 prio = __bsf(mask);
    const int32 index = this->head[prio] - 1;
    __store_release(&this->head[prio], index);
    Task* task = this->tasks[prio][index % elemNum];
    IF_TASK_STATISTICS(statGetNum++);
    return task;
  }

  // Idem
  template<int elemNum>
  Task* TaskWorkStealingQueue<elemNum>::steal(void) {
    if (this->getActiveMask() == 0) return NULL;
    Lock<typename TaskQueue<elemNum>::MutexType> lock(this->mutex);
    const int mask = this->getActiveMask();
    if (mask == 0) return NULL;
    const uint32 prio = __bsf(mask);
    const int32 index = this->tail[prio];
    Task* stolen = this->tasks[prio][index % elemNum];
    this->tail[prio]++;
    IF_TASK_STATISTICS(statStealNum++);
    return stolen;
  }

  // insertion is done by all threads. We use a mutex
  template<int elemNum>
  bool TaskAffinityQueue<elemNum>::insert(Task &task) {
    const uint32 prio = task.getPriority();
    if (UNLIKELY(this->head[prio] - this->tail[prio] == elemNum))
      return false;
    Lock<typename TaskQueue<elemNum>::MutexType> lock(this->mutex);
    if (UNLIKELY(this->head[prio] - this->tail[prio] == elemNum))
      return false;
    __store_release(&task.state, uint8(TaskState::READY));
    __store_release(&this->tasks[prio][this->head[prio] % elemNum], &task);
    const int32 nextHead = this->head[prio] + 1;
    __store_release(&this->head[prio], nextHead);
    IF_TASK_STATISTICS(statInsertNum++);
    return true;
  }

  // get is only done by the owner that therefore owns the tail. We use proper
  // store_release / load_acquire to avoid locks
  template<int elemNum>
  Task* TaskAffinityQueue<elemNum>::get(void) {
    if (this->getActiveMask() == 0) return NULL;
    const int mask = this->getActiveMask();
    const uint32 prio = __bsf(mask);
    Task* task = __load_acquire(&this->tasks[prio][this->tail[prio] % elemNum]);
    const int32 nextTail = this->tail[prio] + 1;
    __store_release(&this->tail[prio], nextTail);
    IF_TASK_STATISTICS(statGetNum++);
    return task;
  }

  TaskAllocator::TaskAllocator(uint32 threadNum_) : threadNum(threadNum_) {
    this->local = PF_NEW_ARRAY(TaskStorage, threadNum);
    for (size_t i = 0; i < threadNum; ++i) this->local[i].allocator = this;
    for (size_t i = 0; i < maxHeap; ++i) this->global[i] = NULL;
  }

  TaskAllocator::~TaskAllocator(void) {
#if PF_TASK_STATICTICS
    uint32 chunkNum = 0;
    for (size_t i = 0; i < threadNum; ++i) {
      this->local[i].printStats();
      chunkNum += uint32(this->local[i].statNewChunkNum);
    }
    std::cout << "Total Memory for Tasks: "
              << double(chunkNum * TaskStorage::chunkSize) / 1024
              << "KB" << std::endl;
#endif /* PF_TASK_STATICTICS */
    int64 allocateNum = 0;
    for (size_t i = 0; i < threadNum; ++i)
      allocateNum += this->local[i].allocateNum;
    //FATAL_IF (allocateNum < 0, "** You may have deleted a task twice **");
    //FATAL_IF (allocateNum > 0, "** You may still hold a reference on a task **");
    PF_DELETE_ARRAY(this->local);
  }

  void *TaskAllocator::allocate(size_t sz) {
    FATAL_IF (sz > maxSize, "Task size is too large (TODO remove that)");
    // We use free list for the task. Each free list node can be made of:
    // [pointer_to_next_node,pointer_to_next_chunk,sizeof(chunk)]
    // We therefore need three times the size of a pointer for the nodes
    // and therefore for the task
    if (sz < 3 * sizeof(void*)) sz = 3 * sizeof(void*);
    return this->local[TaskScheduler::threadID].allocate(sz);
  }

  void TaskAllocator::deallocate(void *ptr) {
    return this->local[TaskScheduler::threadID].deallocate(ptr);
  }

  void TaskStorage::newChunk(uint32 chunkID) {
    IF_TASK_STATISTICS(statNewChunkNum++);
    // We store the size of the elements in the chunk header
    const uint32 elemSize = 1 << chunkID;
    char *chunk = (char *) PF_ALIGNED_MALLOC(chunkSize, chunkSize);

    // We store this pointer to free it later while deleting the task
    // allocator
    this->toFree.push_back(chunk);
    *(uint32 *) chunk = elemSize;

    // Fill the free list here
    this->currSize[chunkID] = elemSize;
    char *data = (char*) chunk + CACHE_LINE;
    const char *end = (char*) chunk + chunkSize;
    *(void**) data = NULL; // Last element of the list is the first in chunk
    void *pred = data;
    data += elemSize;
    while (data + elemSize <= end) {
      *(void**) data = pred;
      pred = data;
      data += elemSize;
      this->currSize[chunkID] += elemSize;
    }
    this->chunk[chunkID] = pred;
  }

  TaskThread::TaskThread(void) :
    state(TASK_THREAD_STATE_RUNNING), victim(0), toWakeUp(0)
#if PF_TASK_STATICTICS
    , sleepNum(0u)
#endif /* PF_TASK_STATICTICS */
  {}

  TaskThread::~TaskThread(void) {
#if PF_TASK_STATICTICS
    std::cout << "Thread " << threadID << " sleepNum: " << sleepNum << std::endl;
#endif /* PF_TASK_STATICTICS */
  }

  void TaskThread::sleep(void) {
    Lock<MutexSys> lock(mutex);

    // Double check that we did not get anything to run in the mean time
    // Note that we always go to sleep if the system is locked
    if (afQueue.getActiveMask() && !scheduler->locked) return;
    if (state == TASK_THREAD_STATE_DEAD) return;

    // Previous state is not necessarily RUNNING. It can be "OUTSIDE"
    const TaskThreadState prevState = state;
    state = TASK_THREAD_STATE_SLEEPING;

    // *Globally* indicate that we are now sleeping
    TASK_PROFILE(scheduler->profiler, onSleep, threadID);
    scheduler->sleepMutex.lock();
    scheduler->sleeping |= (size_t(1u) << this->threadID);
    scheduler->sleepingNum++;
    scheduler->sleepMutex.unlock();
    IF_TASK_STATISTICS(this->sleepNum++);
    while (state == TASK_THREAD_STATE_SLEEPING)
      cond.wait(mutex);

    // We are not sleeping anymore. Return to our previous state
    scheduler->sleepMutex.lock();
    scheduler->sleepingNum--;
    scheduler->sleeping &= ~(size_t(1u) << this->threadID);
    scheduler->sleepMutex.unlock();
    
    // We got killed
    if (state == TASK_THREAD_STATE_DEAD) return;
    state = prevState;
  }

  void TaskThread::wakeUp(int32 threadThatWakesMeUp) {
    Lock<MutexSys> lock(mutex);
    if (state == TASK_THREAD_STATE_SLEEPING) {
      TASK_PROFILE(scheduler->profiler, onWakeUp, threadID);
      if (threadThatWakesMeUp >= 0)
        victim = threadThatWakesMeUp;
      state = TASK_THREAD_STATE_RUNNING;
      cond.broadcast();
    }
  }

  void TaskThread::tryWakeUp(int32 threadThatWakesMeUp) {
    if (state != TASK_THREAD_STATE_SLEEPING) return;
    this->wakeUp(threadThatWakesMeUp);
  }

  void TaskThread::die(void) {
    Lock<MutexSys> lock(mutex);
    state = TASK_THREAD_STATE_DEAD;
    cond.broadcast();
  }

  void TaskStorage::pushGlobal(uint32 chunkID) {
    IF_TASK_STATISTICS(statPushGlobalNum++);

    const uint32 elemSize = 1 << chunkID;
    void *list = this->chunk[chunkID];
    void *succ = list, *pred = NULL;
    uintptr_t totalSize = 0;
    while (this->currSize[chunkID] > chunkSize) {
      assert(succ);
      pred = succ;
      succ = *(void**) succ;
      this->currSize[chunkID] -= elemSize;
      totalSize += elemSize;
    }

    // If we pull off some nodes, then we push them back to the global heap
    if (pred) {
      *(void**) pred = NULL;
      this->chunk[chunkID] = succ;
      Lock<MutexActive> lock(allocator->mutex);
      ((void**) list)[1] = allocator->global[chunkID];
      ((uintptr_t *) list)[2] = totalSize;
      allocator->global[chunkID] = list;
    }
  }

  void TaskStorage::popGlobal(uint32 chunkID) {
    void *list = NULL;
    assert(this->chunk[chunkID] == NULL);
    if (allocator->global[chunkID] == NULL) return;

    // Limit the contention time
    do {
      Lock<MutexActive> lock(allocator->mutex);
      list = allocator->global[chunkID];
      if (list == NULL) return;
      allocator->global[chunkID] = ((void**) list)[1];
    } while (0);

    // This is our new chunk
    this->chunk[chunkID] = list;
    this->currSize[chunkID] = uint32(((uintptr_t *) list)[2]);
    IF_TASK_STATISTICS(statPopGlobalNum++);
  }

  void* TaskStorage::allocate(size_t sz) {
    IF_TASK_STATISTICS(statAllocateNum++);
    const uint32 chunkID = __bsf(int(nextHighestPowerOf2(uint32(sz))));
    if (UNLIKELY(this->chunk[chunkID] == NULL)) {
      this->popGlobal(chunkID);
      if (UNLIKELY(this->chunk[chunkID] == NULL))
        this->newChunk(chunkID);
    }
    void *curr = this->chunk[chunkID];
    this->chunk[chunkID] = *(void**) curr; // points to its predecessor
    this->currSize[chunkID] -= 1 << chunkID;
    this->allocateNum++;
    return curr;
  }

  void TaskStorage::deallocate(void *ptr) {
    IF_TASK_STATISTICS(statDeallocateNum++);
    // Figure out with the chunk header the size of this element
    char *chunk = (char*) (uintptr_t(ptr) & ~((1<<logChunkSize)-1));
    const uint32 elemSize = *(uint32*) chunk;
    const uint32 chunkID = __bsf(int(nextHighestPowerOf2(uint32(elemSize))));

    // Insert the free element in the free list
    void *succ = this->chunk[chunkID];
    *(void**) ptr = succ;
    this->chunk[chunkID] = ptr;
    this->currSize[chunkID] += elemSize;

    // If this thread has too many free tasks, we give some to the global heap
    if (this->currSize[chunkID] > 2 * chunkSize)
      this->pushGlobal(chunkID);
    this->allocateNum--;
  }

  void TaskScheduler::threadFunction(TaskScheduler::ThreadStartup *threadData)
  {
    threadID = uint32(threadData->tid);
    TaskScheduler *This = &threadData->scheduler;
    TaskThread &myself = This->taskThread[threadID];
    const int maxInactivityNum = (This->getWorkerNum()+1) * PF_TASK_TRIES_BEFORE_YIELD;
    int inactivityNum = 0;

    // We do not need it anymore
    PF_DELETE(threadData);

    // flush to zero and no denormals
    _mm_setcsr(_mm_getcsr() | (1<<15) | (1<<6));

    // We try to pick up a task from our queue and then we try to steal a task
    // from other queues
    for (;;) {
      Task *task = This->getTask();
      if (task) {
        This->runTask(task);
        inactivityNum = 0;
      } else
        inactivityNum++; 
      if (UNLIKELY(myself.state == TASK_THREAD_STATE_DEAD)) break;
      if (UNLIKELY(inactivityNum >= maxInactivityNum)) {
        inactivityNum = 0;
        myself.sleep();
      }
      while (UNLIKELY(This->locked))
        myself.sleep();
    }
  }

  TaskScheduler::TaskScheduler(int workerNum_) :
    taskThread(NULL),
#if PF_TASK_PROFILER
    profiler(NULL),
#endif /* PF_TASK_PROFILER */      
    sleeping(0u), sleepingNum(0), locked(0)
  {
    if (workerNum_ < 0) workerNum_ = getNumberOfLogicalThreads() - 1;
    this->workerNum = workerNum_;

    // We have a work queue for the main thread too
    this->queueNum = workerNum+1;
    this->taskThread = PF_NEW_ARRAY(TaskThread, queueNum);
    this->taskThread[PF_TASK_MAIN_THREAD].thread = NULL;
    this->taskThread[PF_TASK_MAIN_THREAD].scheduler = this;
    this->taskThread[PF_TASK_MAIN_THREAD].threadID = 0;
    this->taskThread[PF_TASK_MAIN_THREAD].state = TASK_THREAD_STATE_OUTSIDE;

    // Only if we have dedicated worker threads
    if (workerNum > 0) {
      const size_t stackSize = 4*MB;
      for (size_t i = 0; i < workerNum; ++i) {
        const int affinity = int(i+1);
        ThreadStartup *threadData = PF_NEW(ThreadStartup,i+1,*this);
        this->taskThread[i+1].scheduler = this;
        this->taskThread[i+1].thread = createThread((pf::thread_func) threadFunction, threadData, stackSize, affinity);
        this->taskThread[i+1].threadID = i+1;
      }
    }
  }

  bool TaskScheduler::trySchedule(Task &task) {
    TaskThread &myself = this->taskThread[this->threadID];
    const uint32 affinity = task.getAffinity();
    bool success;
    if (affinity >= this->queueNum) {
      success = myself.wsQueue.insert(task);
      // Wake up one sleeping thread (if any)
      if (success) {
        // no race condition...
        const size_t nonVolatileSleeping = this->sleeping;
        if (UNLIKELY(nonVolatileSleeping)) {
          const size_t sleepingID = __bsf(nonVolatileSleeping);
          assert(sleepingID < this->queueNum);
          this->taskThread[sleepingID].tryWakeUp(threadID);
        }
      }
    } else {
      success = this->taskThread[affinity].afQueue.insert(task);
      // We really have to wake up this thread if not running
      if (success) this->taskThread[affinity].wakeUp();
    }
    return success;
  }

  void TaskScheduler::schedule(Task &task) {
    // We pick up any tasks to make some free space for the task we are
    // scheduling
    while (UNLIKELY(!this->trySchedule(task))) {
      Task *someTask = this->getTask();
      if (someTask) this->runTask(someTask);
    }
  }

  void TaskScheduler::lock(void) {
    // If somebody locked the system, we sleep
    while (atomic_cmpxchg(&this->locked, 1, 0) != 0) {
      TaskThread &myself = this->taskThread[threadID];
      myself.sleep();
    }

    // Everyone goes to sleep except us. Busy waiting is just simpler and
    // locking is anyway super expensive. So, let's do it like this
    while (this->sleepingNum != this->queueNum - 1) _mm_pause();

    // Now we are alone in the world now
    TASK_PROFILE(this->profiler, onLock, threadID);
  }

  void TaskScheduler::unlock(void) {
    // OK. Once again, we do it brutally. We just wake up everybody even the
    // threads that were sleeping before the lock was taken. Expensive but
    // simpler and locking is anyway expensive
    __store_release(&this->locked, 0);
    for (size_t i = 0; i < this->queueNum; ++i) {
      TaskThread &thread = this->taskThread[i];
      thread.wakeUp();
    }
    TASK_PROFILE(this->profiler, onUnlock, threadID);
  }

  TaskScheduler::~TaskScheduler(void) {
    for (size_t i = 0; i < workerNum; ++i)
      join(taskThread[i+1].thread); // thread[0] is main
#if PF_TASK_STATICTICS
    for (size_t i = 0; i < queueNum; ++i) {
      std::cout << "Work Stealing Task Queue " << i << " ";
      taskThread[i].wsQueue.printStats();
    }
    for (size_t i = 0; i < queueNum; ++i) {
      std::cout << "Affinity Task Queue " << i << " ";
      taskThread[i].afQueue.printStats();
    }
#endif /* PF_TASK_STATICTICS */
    PF_SAFE_DELETE_ARRAY(taskThread);
  }

  THREAD uint32 TaskScheduler::threadID = 0;

  Task* TaskScheduler::getTask() {
    Task *task = NULL;
    int32 afMask = this->taskThread[this->threadID].afQueue.getActiveMask();
    int32 wsMask = this->taskThread[this->threadID].wsQueue.getActiveMask();
    // There is one task in our own queues. We try to pick up the one with the
    // highest priority accross the 2 queues
    if (wsMask | afMask) {
      // Avoid the zero corner case by adding a 1 in the last bit
      wsMask |= 0x1u << 31u;
      afMask |= 0x1u << 31u;
      // Case 0: Go in the work stealing queue
      if (__bsf(wsMask) < __bsf(afMask)) {
        task = this->taskThread[this->threadID].wsQueue.get();
        if (task) return task;
      // Case 1: Go in the affinity queue
      } else {
        task = this->taskThread[this->threadID].afQueue.get();
        if (task) return task;
      }
    }
    if (task == NULL) {
      // Case 2: try to steal some task from another thread
      const uint32 victimID = this->taskThread[this->threadID].victim % queueNum;
      this->taskThread[this->threadID].victim++;
      return this->taskThread[victimID].wsQueue.steal();
    }
    return task;
  }

  void TaskScheduler::runTask(Task *task) {
    // Execute the function
    Task *nextToRun = NULL;
    do {
      // Note that the task may already be running if this is a task set (task
      // sets can be run concurrently by several threads).
#ifndef NDEBUG
      const uint8 state = __load_acquire(&task->state);
      assert(state == TaskState::READY || state == TaskState::RUNNING);
#endif /* NDEBUG */
      __store_release(&task->state, uint8(TaskState::RUNNING));
      TASK_PROFILE(this->profiler, onRunStart, task->name, threadID);
      nextToRun = task->run();
      TASK_PROFILE(this->profiler, onRunEnd, task->name, threadID);
      Task *toRelease = task;

      // Explore the completions and runs all continuations if any
      do {
        // We are done here
        if (--task->toEnd == 0) {
          __store_release(&task->state, uint8(TaskState::DONE));
          TASK_PROFILE(this->profiler, onEnd, task->name, threadID);
          // Start the tasks if they become ready
          if (task->toBeStarted) {
            if (--task->toBeStarted->toStart == 0)
              this->schedule(*task->toBeStarted);
          }
          // Traverse all completions to signal we are done
          task = task->toBeEnded.ptr;
        }
        else
          task = NULL;
      } while (task);

      // Now the run function is done, we can remove the scheduler reference
      if (toRelease->refDec()) PF_DELETE(toRelease);

      // Handle the tasks directly passed by the user
      if (nextToRun) {
        assert(nextToRun->state == TaskState::NEW);

        // Careful with affinities: we can run the task on one specific thread
        const uint32 affinity = nextToRun->getAffinity();
        if (UNLIKELY(affinity < this->queueNum)) {
          if (affinity != this->getThreadID()) {
            nextToRun->scheduled();
            nextToRun = NULL;
          }
        }

        // Check dependencies. Only run a task which is ready
        if (nextToRun && nextToRun->toStart > 1) {
          nextToRun->scheduled();
          nextToRun = NULL;
        }
      }
      task = nextToRun;
      if (task) __store_release(&task->state, uint8(TaskState::READY));
    } while (task);
  }

  void TaskScheduler::go(void)
  {
    TaskThread &myself = this->taskThread[PF_TASK_MAIN_THREAD];
    // Be sure that nobody already killed us before we can start
    myself.mutex.lock();
    const uint32 state = myself.state;
    PF_ASSERT(state == TASK_THREAD_STATE_OUTSIDE ||
              state == TASK_THREAD_STATE_DEAD);

    // We were killed, directly exit
    if (state == TASK_THREAD_STATE_DEAD)
      myself.mutex.unlock();
    // Nobody killed us. We can enter the tasking system
    else {
      ThreadStartup *thread = PF_NEW(ThreadStartup, PF_TASK_MAIN_THREAD, *this);
      myself.state = TASK_THREAD_STATE_RUNNING;
      myself.mutex.unlock();
      threadFunction(thread);
    }

    // Properly indicate that we are not in the tasking system anymore
    myself.mutex.lock();
    myself.state = TASK_THREAD_STATE_OUTSIDE;
    myself.mutex.unlock();
  }

  void TaskScheduler::wait(Ref<Task> task) {
    TaskThread &myself = taskThread[PF_TASK_MAIN_THREAD];
    PF_ASSERT(threadID == PF_TASK_MAIN_THREAD);
    PF_ASSERT(myself.state == TASK_THREAD_STATE_OUTSIDE);
    if (LIKELY(task)) {
      while (__load_acquire(&task->state) != TaskState::DONE) {
        Ref<Task> someTask = this->getTask();
        if (someTask) this->runTask(someTask);
        while (UNLIKELY(this->locked)) myself.sleep();
      }
    }
  }

  void TaskScheduler::waitAll(void) {
    TaskThread &myself = taskThread[PF_TASK_MAIN_THREAD];
    PF_ASSERT(threadID == PF_TASK_MAIN_THREAD);
    PF_ASSERT(myself.state == TASK_THREAD_STATE_OUTSIDE);
    for (;;) {
      Task *task = this->getTask();
      if (task) this->runTask(task);
      while (UNLIKELY(this->locked)) myself.sleep();
      if (task == NULL && this->sleepingNum == this->queueNum - 1)
        return;
    }
  }

  static TaskScheduler *scheduler = NULL;
  static TaskAllocator *allocator = NULL;

  void Task::scheduled(void) {
    __store_release(&this->state, uint8(TaskState::SCHEDULED));
    if (--this->toStart == 0) scheduler->schedule(*this);
  }

#if PF_TASK_USE_DEDICATED_ALLOCATOR
  void *Task::operator new(size_t size) { return allocator->allocate(size); }
  void Task::operator delete(void *ptr) { allocator->deallocate(ptr); }
#else
  void *Task::operator new(size_t size) { return alignedMalloc(size, 16); }
  void Task::operator delete(void *ptr) { alignedFree(ptr); }
#endif /* PF_TASK_USE_DEDICATED_ALLOCATOR */
  static void * const fake = NULL;
  void* Task::operator new[](size_t size) { NOT_IMPLEMENTED; return fake; }
  void  Task::operator delete[](void* ptr){ NOT_IMPLEMENTED; }

  Task* TaskSet::run(void)
  {
    // The basic idea with task sets is to reschedule the task in its own
    // queue to have it stolen by another thread. Once done, we simply execute
    // the code of the task set run function concurrently with other threads.
    // The only downside of this approach is that the rescheduled task *must*
    // be picked up again to completely end the task set. This basically may
    // slightly delay the ending of it (note that much since we enqueue /
    // dequeue in LIFO style here)
    // Also, note that we reenqueue the task twice since it allows an
    // exponential propagation of the task sets in the other thread queues
    atomic_t curr;
    if (this->elemNum > 2) {
      this->toEnd += 2;
      this->refInc(); // One more reference in the scheduler
      scheduler->schedule(*this);
      // This is a bit tricky here. Basically, in the case the queue is full, we
      // don't want to recurse and pick up the task set we just scheduled
      // before. This may lead to an infinite recursion. So the second
      // scheduling is only a try
      this->refInc();
      if (UNLIKELY(!scheduler->trySchedule(*this))) {
        this->toEnd--;
        this->refDec();
      }
      while ((curr = --this->elemNum) >= 0) this->run(curr);
    } else if (this->elemNum > 1) {
      this->toEnd++;
      this->refInc(); // One more reference in the scheduler
      scheduler->schedule(*this);
      while ((curr = --this->elemNum) >= 0) this->run(curr);
    } else if (--this->elemNum == 0)
      this->run(0);
    return NULL;
  }

  void TaskingSystemStart(int32 workerNum) {
    static const uint32 bitsPerByte = 8;
    FATAL_IF (workerNum >= int32(sizeof(size_t)*bitsPerByte), "Too many workers are required");
    FATAL_IF (scheduler != NULL, "scheduler is already running");
    // flush to zero and no denormals
    _mm_setcsr(_mm_getcsr() | (1<<15) | (1<<6));
    scheduler = PF_NEW(TaskScheduler, workerNum);
    allocator = PF_NEW(TaskAllocator, scheduler->getWorkerNum()+1);
  }

  void TaskingSystemEnd(void) {
    scheduler->waitAll();  // Empty the queues (ie wait for all tasks)
    scheduler->stopAll();      // Kill all the threads
    PF_SAFE_DELETE(scheduler); // Deallocate the scheduler
    PF_SAFE_DELETE(allocator); // Release the tasks allocator
    scheduler = NULL;
    allocator = NULL;
  }

  void TaskingSystemEnter(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->go();
  }

  void TaskingSystemWait(Ref<Task> task) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->wait(task);
  }

  void TaskingSystemWaitAll(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->waitAll();
  }

  void TaskingSystemLock(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->lock();
  }

  void TaskingSystemUnlock(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->unlock();
  }

  void TaskingSystemInterruptMain(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->stopMain();
  }

  uint32 TaskingSystemGetThreadNum(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    return scheduler->getWorkerNum() + 1;
  }

  uint32 TaskingSystemGetThreadID(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    return scheduler->getThreadID();
  }

#if PF_TASK_PROFILER
  void TaskingSystemSetProfiler(TaskProfiler *profiler) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    TaskingSystemLock();
    scheduler->setProfiler(profiler);
    TaskingSystemUnlock();
  }
#endif /* PF_TASK_PROFILER */
}

#undef IF_TASK_STATISTICS
#undef IF_TASK_PROFILER

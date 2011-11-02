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

#include <vector>
#include <cstdlib>

// One important remark about reference counting. Tasks are referenced
// counted but we do not use Ref<Task> here. This is for performance reasons.
// We therefore *manually* handle the extra references the scheduler may have
// on a task. This is nasty but maintains a reasonnable speed in the system.
// Using Ref<Task> in the queues makes the system roughly twice slower
#if PF_TASK_STATICTICS
#define IF_TASK_STATISTICS(EXPR) EXPR
#else
#define IF_TASK_STATISTICS(EXPR)
#endif

namespace pf {

  ///////////////////////////////////////////////////////////////////////////
  /// Declaration of the internal classes of the tasking system
  ///////////////////////////////////////////////////////////////////////////
  class Task;          // Basically an asynchronous function with dependencies
  class TaskSet;       // Idem but can be run N times
  class TaskAllocator; // Dedicated to allocate tasks and task sets

  /*! Fast random number generator (TODO find something that works) */
  struct CACHE_LINE_ALIGNED FastRand {
    FastRand(void) { z = rand(); }
    INLINE unsigned long rand(void) { return ++z; }
    unsigned long z;
    ALIGNED_CLASS
  };

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
      const __m128i t = __load_acquire(&tail.v);
      const __m128i h = __load_acquire(&head.v);
      const __m128i len = _mm_sub_epi32(t, h);
      return _mm_movemask_ps(_mm_castsi128_ps(len));
    }

  protected:
    Task * tasks[TaskPriority::NUM][elemNum]; //!< All tasks currently stored
    MutexActive mutex;                        //!< Not lock-free right now
    union {
      INLINE volatile int32& operator[] (int32 prio) { return x[prio]; }
      volatile int32 x[TaskPriority::NUM];
      volatile __m128i v;
    } head, tail;
    ALIGNED_CLASS;
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

  /*! Handle the scheduling of all tasks. We basically implement here a
   *  work-stealing algorithm. Each thread has each own queue where it picks
   *  up task in depth first order. Each thread can also steal other tasks
   *  in breadth first order when his own queue is empty
   */
  class TaskScheduler {
  public:

    /*! If threadNum == 0, use the maximum number of threads */
    TaskScheduler(int threadNum_ = -1);
    ~TaskScheduler(void);

    /*! Call by the main thread to enter the tasking system */
    template <bool isMainThread> void go(void);
    /*! Interrupt all threads */
    INLINE void stopAll(void) {
      __store_release(&deadMain, true);
      __store_release(&dead, true);
    }
    /*! Interrupt main thread only */
    INLINE void stopMain(void) { __store_release(&deadMain, true); }
    /*! Number of threads running in the scheduler (not including main) */
    INLINE uint32 getWorkerNum(void) { return uint32(this->workerNum); }
    /*! ID of the calling thread in the tasking system */
    INLINE uint32 getThreadID(void) { return uint32(this->threadID); }
    /*! Try to get a task from all the current queues */
    INLINE Task* getTask(void);
    /*! Run the task and recursively handle the tasks to start and to end */
    void runTask(Task *task);
    /*! Data provided to each thread */
    struct Thread {
      Thread (size_t tid, TaskScheduler &scheduler_) :
        tid(tid), scheduler(scheduler_) {}
      size_t tid;
      TaskScheduler &scheduler;
    };

  private:

    /*! Function run by each thread */
    template <bool isMainThread> static void threadFunction(Thread *thread);
    /*! Schedule a task which is now ready to execute */
    INLINE void schedule(Task &task);
    /*! Try to push a task in the queue. Returns false if queues are full */
    INLINE bool trySchedule(Task &task);

    friend class Task;            //!< Tasks ...
    friend class TaskSet;         // ... task sets ...
    friend class TaskAllocator;   // ... task allocator use the tasking system
    enum { queueSize = 512 };     //!< Number of task per queue
    static THREAD uint32 threadID;//!< ThreadID for each thread
    TaskWorkStealingQueue<queueSize> *wsQueues;//!< 1 queue per thread
    TaskAffinityQueue<queueSize> *afQueues;    //!< 1 queue per thread
    FastRand *random;             //!< 1 random generator per thread
    thread_t *threads;            //!< All threads currently running
    size_t workerNum;             //!< Total number of threads running
    size_t queueNum;              //!< Number of queues (should be workerNum+1)
    volatile bool dead;           //!< The tasking system should quit
    volatile bool deadMain;       //!< The main thread should return
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
                                 //   allocated elsewhere
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
  class TaskAllocator {
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

  // Well, regarding the implementation of the two task queues (work stealing
  // queues and FIFO affinity queues), this code is not really portable and
  // somehow x86 specific. There is no fence and there is no lock when the
  // thread is modifying the head (for WS queues) and the tail (for FIFO
  // affinity queue). At least, a fast ABP lock free queue
  // is clearly the way to go anyway for the works stealing part. Other than
  // that, this code can be ported to other platforms but some fences should be
  // added around volatile reads/writes. The thing is that x86s use a very
  // strong memory model. As a reminder:
  // - Loads are not reordered with other loads
  // - Stores are not reordered with other stores
  // - Stores are not reordered with older loads
  template<int elemNum>
  bool TaskWorkStealingQueue<elemNum>::insert(Task &task) {
    const uint32 prio = task.getPriority();
    if (UNLIKELY(this->head[prio] - this->tail[prio] == elemNum))
      return false;
    __store_release(&task.state, uint8(TaskState::READY));
    PF_COMPILER_READ_WRITE_BARRIER;
    this->tasks[prio][this->head[prio] % elemNum] = &task;
    PF_COMPILER_READ_WRITE_BARRIER;
    this->head[prio]++;
    IF_TASK_STATISTICS(statInsertNum++);
    return true;
  }

  template<int elemNum>
  Task* TaskWorkStealingQueue<elemNum>::get(void) {
    if (this->getActiveMask() == 0) return NULL;
    Lock<MutexActive> lock(this->mutex);
    const int mask = this->getActiveMask();
    if (mask == 0) return NULL;
    const uint32 prio = __bsf(mask);
    const int32 index = --this->head[prio];
    Task* task = this->tasks[prio][index % elemNum];
    IF_TASK_STATISTICS(statGetNum++);
    return task;
  }

  template<int elemNum>
  Task* TaskWorkStealingQueue<elemNum>::steal(void) {
    if (this->getActiveMask() == 0) return NULL;
    Lock<MutexActive> lock(this->mutex);
    const int mask = this->getActiveMask();
    if (mask == 0) return NULL;
    const uint32 prio = __bsf(mask);
    const int32 index = this->tail[prio];
    Task* stolen = this->tasks[prio][index % elemNum];
    this->tail[prio]++;
    IF_TASK_STATISTICS(statStealNum++);
    return stolen;
  }

  template<int elemNum>
  bool TaskAffinityQueue<elemNum>::insert(Task &task) {
    const uint32 prio = task.getPriority();
    // No double check here (I mean, before and after the lock. We just take the
    // optimistic approach ie we suppose the queue is never full)
    Lock<MutexActive> lock(this->mutex);
    if (UNLIKELY(this->head[prio] - this->tail[prio] == elemNum))
      return false;
    __store_release(&task.state, uint8(TaskState::READY));
    PF_COMPILER_READ_WRITE_BARRIER;
    this->tasks[prio][this->head[prio] % elemNum] = &task;
    PF_COMPILER_READ_WRITE_BARRIER;
    this->head[prio]++;
    IF_TASK_STATISTICS(statInsertNum++);
    return true;
  }

  template<int elemNum>
  Task* TaskAffinityQueue<elemNum>::get(void) {
    if (this->getActiveMask() == 0) return NULL;
    Lock<MutexActive> lock(this->mutex);
    const int mask = this->getActiveMask();
    const uint32 prio = __bsf(mask);
    Task* task = this->tasks[prio][this->tail[prio] % elemNum];
    this->tail[prio]++;
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
      chunkNum += this->local[i].statNewChunkNum;
    }
    std::cout << "Total Memory for Tasks: "
              << double(chunkNum * TaskStorage::chunkSize) / 1024
              << "KB" << std::endl;
#endif /* PF_TASK_STATICTICS */
    int64 allocateNum = 0;
    for (size_t i = 0; i < threadNum; ++i)
      allocateNum += this->local[i].allocateNum;
    FATAL_IF (allocateNum < 0, "** You may have deleted a task twice **");
    FATAL_IF (allocateNum > 0, "** You may still hold a reference on a task **");
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

  // Stupid windows
  template <typename T> INLINE T _min(T x, T y) {return x<y?x:y;}
  template <typename T> INLINE T _max(T x, T y) {return x>y?x:y;}

  template <bool isMainThread>
  void TaskScheduler::threadFunction(TaskScheduler::Thread *thread)
  {
    threadID = uint32(thread->tid);
    TaskScheduler *This = &thread->scheduler;
    const int maxInactivityNum = (This->getWorkerNum()+1) * PF_TASK_TRIES_BEFORE_YIELD;
    int inactivityNum = 0;
    int yieldTime = 0;

    // We try to pick up a task from our queue and then we try to steal a task
    // from other queues
    for (;;) {
      Task *task = This->getTask();
      if (task) {
        This->runTask(task);
        yieldTime = inactivityNum = 0;
      } else
        inactivityNum++;
      bool isDead;
      if (isMainThread)
        isDead = __load_acquire(&This->deadMain);
      else
        isDead = __load_acquire(&This->dead);
      if (UNLIKELY(isDead)) break;
      if (UNLIKELY(inactivityNum >= maxInactivityNum)) {
        inactivityNum = 0;
        yield(yieldTime);
        yieldTime = _max(yieldTime, 1);
        yieldTime = _min(PF_TASK_MAX_YIELD_TIME, yieldTime<<1);
      }
    }
    PF_DELETE(thread);
  }

  // Explicitely instantiate both versions of the function
  template void TaskScheduler::threadFunction<false>(TaskScheduler::Thread*);
  template void TaskScheduler::threadFunction<true>(TaskScheduler::Thread*);

  TaskScheduler::TaskScheduler(int workerNum_) :
    wsQueues(NULL), afQueues(NULL), threads(NULL), dead(false), deadMain(false)
  {
    if (workerNum_ < 0) workerNum_ = getNumberOfLogicalThreads() - 1;
    this->workerNum = workerNum_;

    // We have a work queue for the main thread too
    this->queueNum = workerNum+1;
    this->wsQueues = PF_NEW_ARRAY(TaskWorkStealingQueue<queueSize>, queueNum);
    this->afQueues = PF_NEW_ARRAY(TaskAffinityQueue<queueSize>, queueNum);

    // Also one random generator for *every* thread
    this->random = PF_NEW_ARRAY(FastRand, queueNum);

    // Only if we have dedicated worker threads
    if (workerNum > 0) {
      this->threads = PF_NEW_ARRAY(thread_t, workerNum);
      const size_t stackSize = 4*MB;
      for (size_t i = 0; i < workerNum; ++i) {
        const int affinity = int(i+1);
        Thread *thread = PF_NEW(Thread,i+1,*this);
        thread_func threadFunc = (thread_func) threadFunction<false>;
        threads[i] = createThread(threadFunc, thread, stackSize, affinity);
      }
    }
  }

  bool TaskScheduler::trySchedule(Task &task) {
    const uint32 affinity = task.getAffinity();
    bool success;
    if (affinity >= this->queueNum)
      success = wsQueues[this->threadID].insert(task);
    else
      success = afQueues[affinity].insert(task);
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

  TaskScheduler::~TaskScheduler(void) {
    if (threads)
      for (size_t i = 0; i < workerNum; ++i)
        join(threads[i]);
    PF_SAFE_DELETE_ARRAY(threads);
#if PF_TASK_STATICTICS
    for (size_t i = 0; i < queueNum; ++i) {
      std::cout << "Task Queue " << i << " ";
      wsQueues[i].printStats();
    }
#endif /* PF_TASK_STATICTICS */
    PF_SAFE_DELETE_ARRAY(wsQueues);
    PF_SAFE_DELETE_ARRAY(afQueues);
    PF_SAFE_DELETE_ARRAY(random);
  }

  THREAD uint32 TaskScheduler::threadID = 0;

  Task* TaskScheduler::getTask() {
    Task *task = NULL;
    int32 afMask = this->afQueues[this->threadID].getActiveMask();
    int32 wsMask = this->wsQueues[this->threadID].getActiveMask();
    // There is one task in our own queues. We try to pick up the one with the
    // highest priority accross the 2 queues
    if (wsMask | afMask) {
      // Avoid the zero corner case by adding a 1 in the last bit
      wsMask |= 0x1u << 31u;
      afMask |= 0x1u << 31u;
      // Case 0: Go in the work stealing queue
      if (__bsf(wsMask) < __bsf(afMask)) {
        task = this->wsQueues[this->threadID].get();
        if (task) return task;
      // Case 1: Go in the affinity queue
      } else {
        task = this->afQueues[this->threadID].get();
        if (task) return task;
      }
    }
    if (task == NULL) {
      // Case 2: try to steal some task from another thread
      const unsigned long index = this->random[this->threadID].rand()%queueNum;
      return this->wsQueues[index].steal();
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
      nextToRun = task->run();
      Task *toRelease = task;

      // Explore the completions and runs all continuations if any
      do {
        const atomic_t stillRunning = --task->toEnd;

        // We are done here
        if (stillRunning == 0) {
          __store_release(&task->state, uint8(TaskState::DONE));
          // Start the tasks if they become ready
          if (task->toBeStarted) {
            task->toBeStarted->toStart--;
            if (task->toBeStarted->toStart == 0)
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
      }
      task = nextToRun;
      if (task) __store_release(&task->state, uint8(TaskState::READY));
    } while (task);
  }

  template<bool isMainThread>
  void TaskScheduler::go(void) {
    Thread *thread = PF_NEW(Thread, 0, *this);
    threadFunction<isMainThread>(thread);
  }

  static TaskScheduler *scheduler = NULL;
  static TaskAllocator *allocator = NULL;

  void Task::scheduled(void) {
    __store_release(&this->state, uint8(TaskState::SCHEDULED));
    this->toStart--;
    if (this->toStart == 0) scheduler->schedule(*this);
  }

#if PF_TASK_USE_DEDICATED_ALLOCATOR
  void *Task::operator new(size_t size) { return allocator->allocate(size); }
  void Task::operator delete(void *ptr) { allocator->deallocate(ptr); }
#endif /* PF_TASK_USE_DEDICATED_ALLOCATOR */

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

  void Task::waitForCompletion(void) {
    while (__load_acquire(&this->state) != TaskState::DONE) {
      Task *someTask = scheduler->getTask();
      if (someTask) scheduler->runTask(someTask);
    }
  }

  void TaskingSystemStart(int32 workerNum) {
    FATAL_IF (scheduler != NULL, "scheduler is already running");
    scheduler = PF_NEW(TaskScheduler, workerNum);
    allocator = PF_NEW(TaskAllocator, scheduler->getWorkerNum()+1);
  }

  void TaskingSystemEnd(void) {
    TaskingSystemInterrupt();
    PF_SAFE_DELETE(scheduler);
    PF_SAFE_DELETE(allocator);
    scheduler = NULL;
    allocator = NULL;
  }

  void TaskingSystemEnter(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->go<true>();
  }

  void TaskingSystemInterruptMain(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->stopMain();
  }

  void TaskingSystemInterrupt(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    scheduler->stopAll();
  }

  uint32 TaskingSystemGetThreadNum(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    return scheduler->getWorkerNum() + 1;
  }

  uint32 TaskingSystemGetThreadID(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    return scheduler->getThreadID();
  }

  bool TaskingSystemRunAnyTask(void) {
    FATAL_IF (scheduler == NULL, "scheduler not started");
    Task *someTask = scheduler->getTask();
    if (someTask) {
      scheduler->runTask(someTask);
      return true;
    } else
      return false;
  }
}

#undef IF_TASK_STATISTICS


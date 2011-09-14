//#include "sys/tasking.hpp"
#include "sys/ref.hpp"
#include "sys/thread.hpp"
#include "sys/mutex.hpp"
#include "sys/sysinfo.hpp"

namespace pf {

  /*! Interface for all tasks handled by the tasking system */
  class Task : public RefCount
  {
  public:

    /*! It can complete one task and can be continued by one other task */
    Task(Task *completion_ = NULL, Task *continuation_ = NULL);

    /*! Now the task is built and immutable */
    void done(void);

    /*! To override while specifying a task */
    virtual void run(void) = 0;

  private:
    friend class TaskSet;       //!< Will tweak the ending criterium
    friend class TaskScheduler; //!< Needs to access everything
    Ref<Task> completion;       //!< Signalled it when finishing
    Ref<Task> continuation;     //!< Triggers it when ready
    Atomic toStart;             //!< MBZ before starting
    Atomic toEnd;               //!< MBZ before ending
  };

  /*! Allow the run function to be executed several times */
  class TaskSet : public Task
  {
  public:

    /*! As for Task, it has both completion and continuation */
    TaskSet(size_t elemNum, Task *completion = NULL, Task *continuation = NULL);

    /*! This function is user-specified */
    virtual void run(size_t elemID) = 0;

  private:
    virtual void run(void);  //!< Reimplemented for all task sets
    Atomic elemNum;          //!< Number of outstanding elements
  };

  /*! Structure used for work stealing */
  template <int elemNum>
  class TaskQueue
  {
  public:
    INLINE TaskQueue(void) : head(0), tail(0) {}
    INLINE void insert(Task &task) {
      assert(atomic_t(head) - atomic_t(tail) < elemNum);
      tasks[atomic_t(head) % elemNum] = &task;
      head++;
    }
    INLINE Ref<Task> get(void) {
      if (head == tail) return NULL;
      Lock<MutexActive> lock(mutex);
      if (head == tail) return NULL;
      head--;
      Ref<Task> task = tasks[head % elemNum];
      tasks[head % elemNum] = NULL;
      return task;
    }
    INLINE Ref<Task> steal(void) {
      if (head == tail) return NULL;
      Lock<MutexActive> lock(mutex);
      if (head == tail) return NULL;
      Ref<Task> stolen = tasks[tail % elemNum];
      tasks[tail % elemNum] = NULL;
      tail++;
      return stolen;
    }
  private:
    Ref<Task> tasks[elemNum]; //!< All tasks currently stored
    Atomic head, tail;        //!< Current queue property
    MutexActive mutex;        //!< Not lock-free right now
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
    void go(void);

    /*! Interrupt all threads */
    INLINE void die(void) { dead = true; }

    /*! Data provided to each thread */
    struct Thread {
      Thread (size_t tid, TaskScheduler &scheduler_) :
        tid(tid), scheduler(scheduler_) {}
      size_t tid;
      TaskScheduler &scheduler;
    };

  private:

    /*! Function run by each thread */
    static void threadFunction(Thread *thread);
    /*! Schedule a task which is now ready to execute */
    void schedule(Task &task);

    friend class Task;            //!< Only tasks ...
    friend class TaskSet;         //   ... and task sets use the tasking system
    static THREAD uint32 threadID;//!< ThreadID for each thread
    enum { queueSize = 2048 };    //!< Number of task per queue
    TaskQueue<queueSize> *queues; //!< One queue per thread
    thread_t *threads;            //!< All threads currently running
    size_t threadNum;             //!< Total number of threads running
    size_t queueNum;              //!< Number of queues (should be threadNum+1)
    volatile bool dead;           //!< The tasking system should quit
  };

  TaskScheduler::TaskScheduler(int threadNum_) :
    queues(NULL), threads(NULL), dead(false)
  {
    if (threadNum_ < 0) threadNum_ = getNumberOfLogicalThreads() - 1;
    this->threadNum = threadNum_;

    // We have a work queue for the main thread too
    this->queueNum = threadNum+1;
    this->queues = NEW_ARRAY(TaskQueue<queueSize>, queueNum);

    // Only if we have dedicated worker threads
    if (threadNum > 0) {
      this->threads = NEW_ARRAY(thread_t, threadNum);
      const size_t stackSize = 4*MB;
      for (size_t i = 0; i < threadNum; ++i) {
        const int affinity = int(i+1);
        Thread *thread = NEW(Thread,i+1,*this);
        thread_func threadFunc = (thread_func) threadFunction;
        threads[i] = createThread(threadFunc, thread, stackSize, affinity);
      }
    }
  }

  void TaskScheduler::schedule(Task &task) {
    queues[this->threadID].insert(task);
  }

  TaskScheduler::~TaskScheduler(void) {
    if (threads)
      for (size_t i = 0; i < threadNum; ++i)
        join(threads[i]);
    SAFE_DELETE_ARRAY(threads);
    SAFE_DELETE_ARRAY(queues);
  }

  THREAD uint32 TaskScheduler::threadID = 0;

  void TaskScheduler::threadFunction(TaskScheduler::Thread *thread)
  {
    threadID = thread->tid;
    TaskScheduler *This = &thread->scheduler;

    // We try to pick up a task from our queue and then we try to steal a task
    // from other queues
    for (;;) {
      Ref<Task> task = This->queues[threadID].get();
      while (!task && !This->dead) {
        for (size_t i = 0; i < threadID; ++i)
          if (task = This->queues[i].steal()) break;
        if (!task)
          for (size_t i = threadID+1; i < This->queueNum; ++i)
            if (task = This->queues[i].steal()) break;
      }
      if (This->dead) break;

      // Execute the function
      task->run();
      // The run is one dependency to finish
      const atomic_t isDone = task->toEnd--;

      // We are done here
      if (isDone == 0) {
        // Run the continuation if any
        if (task->continuation) {
          task->continuation->toStart--;
          if (task->continuation->toStart == 0)
            This->schedule(*task->continuation);
        }
        // Traverse all completions to signal we are done
        Ref<Task> completion = task->completion;
        while (completion) {
          completion->toEnd--;
          if (completion->toEnd == 0)
            completion = completion->completion;
          else
            completion = NULL;
        }
      }
    }
    DELETE(thread);
  }

  void TaskScheduler::go(void) {
    Thread *thread = NEW(Thread, 0, *this);
    threadFunction(thread);
  }

  static TaskScheduler *scheduler = NULL;

  void Task::done(void) {
    this->toStart--;
    if (this->toStart == 0) scheduler->schedule(*this);
  }

  Task::Task(Task *completion_, Task *continuation_) :
    completion(completion_),
    continuation(continuation_),
    toStart(1), toEnd(1)
  {
    if (continuation) continuation->toStart++;
    if (completion) completion->toEnd++;
  }

  void TaskSet::run(void)
  {
    // The basic idea with task sets is to reschedule the task in its own
    // queue to have it stolen by another thread. Once done, we simply execute
    // the code of the task set run function concurrently with other threads.
    // The only downside of this approach is that the reschedule task *must*
    // be picked up again to completely end the task set. This basically may
    // slightly delay the ending of it (note that much since we enqueue /
    // dequeue in LIFO style here)
    if (this->elemNum > 1) {
      this->toEnd++;
      scheduler->schedule(*this);
      atomic_t curr;
      while ((curr = --this->elemNum) >= 0)
        this->run(curr);
    }
  }
}

using namespace pf;

class NothingTask : public Task {
public:
  NothingTask(Task *completion = NULL, Task *continuation = NULL) :
    Task(completion, continuation) {}
  virtual void run(void) {
  }
};

class DoneTask : public Task {
public:
  DoneTask(Task *completion = NULL, Task *continuation = NULL) :
    Task(completion, continuation) {}
  virtual void run(void) { scheduler->die(); }
};

int main(int argc, char **argv)
{
  startMemoryDebugger();
  scheduler = NEW(TaskScheduler, 2);
  Task *done = NEW(DoneTask);
  Task *nothing = NEW(NothingTask, NULL, done);
  done->done();
  nothing->done();
  scheduler->go();

  DELETE(scheduler);
  scheduler = NULL;
  endMemoryDebugger();
  return 0;
}


#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include "Semaphore.h"

using namespace std;

typedef struct worker {
    thread ts;
    Semaphore ready{0};
    function<void(void)> task;
    atomic<bool> available{true};
} worker_t;

class ThreadPool {
  public:
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

  private:
    void worker(int id);
    void dispatcher();

    thread dt;
    vector<worker_t> wts;
    queue<function<void(void)>> tasks;

    atomic<bool> done;
    int pendingTasks = 0;

    mutex queueLock;
    condition_variable_any taskFree;
    condition_variable_any allDone;

    mutex workersLock;
    condition_variable_any workerFree;
};

#endif

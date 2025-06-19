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
#include "Semaphore.h"

using namespace std;

typedef struct worker {
    thread ts;
    function<void(void)> thunk;
    bool available;
    Semaphore semaphore{0};
    int id;
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

    // Miembros en orden de declaración
    atomic<bool> done{false};
    atomic<bool> destructionStarted{false};
    mutex queueLock;
    condition_variable_any queueCV;
    Semaphore availableWorkers;
    atomic<int> pendingTasks{0};
    condition_variable_any waitCV;
    mutex waitLock;
    thread dt;
    vector<worker_t> wts;
    queue<function<void(void)>> tasks;
};

#endif
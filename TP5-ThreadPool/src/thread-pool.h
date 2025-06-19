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

using namespace std;

struct worker_t {
    int id;
    thread ts;
    function<void()> task;
    bool ready = false;
    mutex mtx;
    condition_variable cv;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

private:
    void worker(int id);
    void dispatcher();

    // Miembros ordenados para evitar warnings
    atomic<bool> done{false};
    atomic<int> pendingTasks{0};
    thread dt;
    vector<worker_t> wts;
    
    queue<function<void()>> tasks;
    mutex taskMutex;
    condition_variable taskAvailable;

    queue<int> idleWorkers;
    mutex wtQueueMutex;
    condition_variable workerReady;

    mutex waitMutex;
    condition_variable waitCond;
};

#endif
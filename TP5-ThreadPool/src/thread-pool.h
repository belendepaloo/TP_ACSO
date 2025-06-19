#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

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
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

    // Evita copia y asignación
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

private:
    void worker(int id);
    void dispatcher();

    bool done;
    int pendingTasks;

    thread dt;                          // Dispatcher thread
    vector<worker_t> wts;               // Worker pool

    queue<function<void()>> tasks;      // Task queue
    mutex taskMutex;                    // Protects task queue
    condition_variable taskAvailable;   // Signals when tasks are available

    queue<int> idleWorkers;             // IDs of available workers
    mutex wtQueueMutex;                 // Protects idleWorkers queue
    condition_variable workerReady;     // Signals when workers are available

    mutex waitMutex;                    // Protects pendingTasks + done
    condition_variable waitCond;        // Notifies wait() when all tasks are done
};

#endif

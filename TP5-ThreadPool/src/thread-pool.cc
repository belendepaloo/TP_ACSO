#include "thread-pool.h"
#include <iostream>
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), availableWorkers(numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].id = i;
        wts[i].available = true;
        wts[i].ts = thread(&ThreadPool::worker, this, i);
    }
    dt = thread(&ThreadPool::dispatcher, this);
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (!thunk) {
        throw invalid_argument("Cannot schedule a null thunk");
    }
    if (destructionStarted.load()) {
        throw runtime_error("Schedule called after destruction started");
    }

    {
        lock_guard<mutex> lock(queueLock);
        if (done) return;
        tasks.push(thunk);
        pendingTasks++;
    }
    queueCV.notify_one();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(waitLock);
    waitCV.wait(lock, [this] { 
        return pendingTasks == 0 || destructionStarted.load(); 
    });
}

ThreadPool::~ThreadPool() {
    destructionStarted = true;

    while (true) {
    {
        lock_guard<mutex> qlock(queueLock);
        lock_guard<mutex> wlock(waitLock);
        if (pendingTasks == 0 && tasks.empty()) break;
    }
    this_thread::yield(); // o sleep(1ms)
}


    {
        lock_guard<mutex> lock(queueLock);
        done = true;
    }

    queueCV.notify_all();

    for (size_t i = 0; i < wts.size(); ++i) {
        availableWorkers.signal();
    }

    for (auto& wt : wts) {
        wt.semaphore.signal();
    }

    {
        lock_guard<mutex> lock(waitLock);
        waitCV.notify_all();
    }

    for (auto& wt : wts) {
        if (wt.ts.joinable()) {
            wt.ts.join();
        }
    }

    if (dt.joinable()) {
        dt.join();
    }
}


void ThreadPool::worker(int id) {
    while (true) {
        // Espera hasta que el dispatcher le asigne trabajo
        wts[id].semaphore.wait();

        if (done) break;

        // Toma la tarea de manera segura
        function<void()> task;
        {
            lock_guard<mutex> taskLock(taskMapMutex);
            task = move(taskMap[id]);
            taskMap.erase(id);
        }

        // Ejecuta la tarea
        if (task) task();

        {
            lock_guard<mutex> lock(queueLock);
            wts[id].available = true;
            pendingTasks--;

            if (pendingTasks == 0) {
                lock_guard<mutex> waitLockGuard(waitLock);
                waitCV.notify_all();
            }

            availableWorkers.signal();
        }
    }
}


void ThreadPool::dispatcher() {
    while (true) {
        {
            unique_lock<mutex> lock(queueLock);
            if (done && tasks.empty()) {
                break;
            }
            
            queueCV.wait(lock, [this] { 
                return !tasks.empty() || done; 
            });
            
            if (done && tasks.empty()) {
                break;
            }

            if (tasks.empty()) {
                continue;
            }
        }

        availableWorkers.wait();

        {
            lock_guard<mutex> lock(queueLock);
            for (auto& wt : wts) {
                if (wt.available) {
                    auto thunk = move(tasks.front());
                    tasks.pop();
                    wt.available = false;

                    {
                        lock_guard<mutex> taskLock(taskMapMutex);
                        taskMap[wt.id] = move(thunk);
                    }

                    wt.semaphore.signal();
                    break;
                }
            }

        }
    }
    
    // Ensure all workers wake up to check done flag
    for (auto& wt : wts) {
        wt.semaphore.signal();
    }
}
#include "thread-pool.h"
#include <iostream>
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : 
    wts(numThreads), 
    availableWorkers(numThreads),
    done(false),
    destructionStarted(false) {
    
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].id = i;
        wts[i].available = true;
        wts[i].ts = thread(&ThreadPool::worker, this, i);
    }
    dt = thread(&ThreadPool::dispatcher, this);
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (!thunk) {
        throw invalid_argument("Cannot schedule nullptr function");
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
    
    // Wait for all tasks to complete
    wait();
    
    {
        lock_guard<mutex> lock(queueLock);
        done = true;
    }
    
    // Wake up all threads
    queueCV.notify_all();
    
    // Signal all workers to exit
    for (auto& wt : wts) {
        wt.semaphore.signal();
    }

    // Join all threads
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
        wts[id].semaphore.wait();

        if (done) break;

        if (wts[id].thunk) {
            try {
                wts[id].thunk();
            } catch (...) {
                // Handle task exceptions
            }
            wts[id].thunk = nullptr;
        }

        {
            lock_guard<mutex> lock(queueLock);
            wts[id].available = true;
            if (wts[id].thunk == nullptr) {
                pendingTasks--;
            }

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
            queueCV.wait(lock, [this] { 
                return !tasks.empty() || done; 
            });
            
            if (done && tasks.empty()) break;
            if (tasks.empty()) continue;
        }

        availableWorkers.wait();

        {
            lock_guard<mutex> lock(queueLock);
            for (auto& wt : wts) {
                if (wt.available) {
                    wt.thunk = move(tasks.front());
                    tasks.pop();
                    wt.available = false;
                    wt.semaphore.signal();
                    break;
                }
            }
        }
    }
}
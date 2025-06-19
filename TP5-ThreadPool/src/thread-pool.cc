#include "thread-pool.h"
#include <iostream>
using namespace std;

#define DEBUG true
#define DOUT if (DEBUG) cout

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), availableWorkers(numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].id = i;
        wts[i].available = true;
        wts[i].ts = thread(&ThreadPool::worker, this, i);
    }
    dt = thread(&ThreadPool::dispatcher, this);
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (destructionStarted.load()) {
        throw runtime_error("Schedule called after destruction started");
    }

    {
        lock_guard<mutex> lock(queueLock);
        DOUT << "[schedule] Adding task. Pending: " << pendingTasks + 1 << endl;
        tasks.push(thunk);
        pendingTasks++;
    }
    queueCV.notify_one();
}

void ThreadPool::wait() {
    DOUT << "[wait] Waiting for all tasks to complete." << endl;
    unique_lock<mutex> lock(waitLock);
    waitCV.wait(lock, [this] { 
        return pendingTasks == 0; 
    });
    DOUT << "[wait] All tasks completed." << endl;
}

ThreadPool::~ThreadPool() {
    destructionStarted = true;
    
    {
        lock_guard<mutex> lock(queueLock);
        done = true;
    }
    
    // Wake up all threads
    queueCV.notify_all();
    availableWorkers.signal();
    
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
        DOUT << "[worker " << id << "] Waiting for task." << endl;
        wts[id].semaphore.wait();

        if (done) {
            DOUT << "[worker " << id << "] Exiting." << endl;
            break;
        }

        DOUT << "[worker " << id << "] Running task." << endl;
        wts[id].thunk();

        {
            lock_guard<mutex> lock(queueLock);
            wts[id].available = true;
            int remaining = --pendingTasks;

            DOUT << "[worker " << id << "] Task done. Remaining: " << remaining << endl;

            if (remaining == 0) {
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
            DOUT << "[dispatcher] Waiting for tasks or shutdown signal." << endl;
            queueCV.wait(lock, [this] { return !tasks.empty() || done; });
            
            if (done && tasks.empty()) {
                DOUT << "[dispatcher] Shutting down." << endl;
                break;
            }

            if (tasks.empty()) {
                DOUT << "[dispatcher] Woke up but no tasks." << endl;
                continue;
            }
        }

        availableWorkers.wait();

        {
            lock_guard<mutex> lock(queueLock);
            for (auto& wt : wts) {
                if (wt.available) {
                    DOUT << "[dispatcher] Assigning task to worker " << wt.id << endl;
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
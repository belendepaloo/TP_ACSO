/**
 * File: thread-pool.cc
 * -------------------
 * Presents the implementation of the ThreadPool class.
 */


#include "thread-pool.h"
using namespace std;
#define DEBUG true
#define DOUT if (DEBUG) cout
#include <iostream>


ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false), availableWorkers(numThreads) {
    // Initialize workers
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].id = i;
        wts[i].available = true;
        wts[i].ts = thread([this, i] { worker(i); });
    }
    
    // Start dispatcher thread
    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
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
    unique_lock<mutex> lock(queueLock);
    waitCV.wait(lock, [this] { return pendingTasks == 0; });
    DOUT << "[wait] All tasks completed." << endl;
}


ThreadPool::~ThreadPool() {
    {
        lock_guard<mutex> lock(queueLock);
        done = true;
        queueCV.notify_all();
    }

    // Notify all workers to exit
    for (auto& wt : wts) {
        wt.semaphore.signal();
    }

    // Join all worker threads
    for (auto& wt : wts) {
        if (wt.ts.joinable()) {
            wt.ts.join();
        }
    }

    // Join dispatcher thread
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
            pendingTasks--;

            DOUT << "[worker " << id << "] Task done. Remaining: " << pendingTasks << endl;

            if (pendingTasks == 0) {
                waitCV.notify_all();
            }

            availableWorkers.signal();
        }
    }
}


void ThreadPool::dispatcher() {
    while (true) {
        function<void(void)> task;
        
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

            DOUT << "[dispatcher] Task available. Waiting for worker." << endl;
        }

        availableWorkers.wait();

        {
            lock_guard<mutex> lock(queueLock);
            for (auto& wt : wts) {
                if (wt.available) {
                    DOUT << "[dispatcher] Assigning task to worker " << wt.id << endl;
                    task = tasks.front();
                    tasks.pop();
                    wt.thunk = task;
                    wt.available = false;
                    wt.semaphore.signal();
                    break;
                }
            }
        }
    }
}

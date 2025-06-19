/**
 * File: thread-pool.cc
 * -------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

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
    lock_guard<mutex> lock(queueLock);
    tasks.push(thunk);
    pendingTasks++;
    queueCV.notify_one();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(queueLock);
    waitCV.wait(lock, [this] { return pendingTasks == 0; });
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
        wts[id].semaphore.wait(); // Wait for work
        
        // Check if we should exit
        if (done) break;
        
        // Execute the task
        wts[id].thunk();
        
        {
            lock_guard<mutex> lock(queueLock);
            wts[id].available = true;
            pendingTasks--;
            
            // Notify wait() if all tasks are done
            if (pendingTasks == 0) {
                waitCV.notify_all();
            }
            
            // Notify dispatcher that a worker is available
            availableWorkers.signal();
        }
    }
}

void ThreadPool::dispatcher() {
    while (true) {
        function<void(void)> task;
        
        {
            unique_lock<mutex> lock(queueLock);
            queueCV.wait(lock, [this] { return !tasks.empty() || done; });
            
            // Check if we should exit
            if (done && tasks.empty()) break;
            if (tasks.empty()) continue;
            
            // Wait for an available worker
            availableWorkers.wait();
            
            // Find an available worker
            for (auto& wt : wts) {
                if (wt.available) {
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
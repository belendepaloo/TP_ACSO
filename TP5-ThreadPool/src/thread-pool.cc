// thread-pool.cc
#include "thread-pool.h"
#include <iostream>
using namespace std;

ThreadPool::ThreadPool(size_t numThreads)
    : wts(numThreads), availableWorkers(numThreads), done(false) {

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
    {
        lock_guard<mutex> lock(doneMutex);
        if (done) {
            throw runtime_error("Schedule called on destroyed ThreadPool");
        }
        pendingTasks++;
    }

    {
        lock_guard<mutex> lock(queueLock);
        tasks.push(thunk);
    }

    queueCV.notify_one();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(doneMutex);
    waitCV.wait(lock, [this]() {
        return pendingTasks == 0;
    });
}

ThreadPool::~ThreadPool() {
    wait();

    {
        lock_guard<mutex> lock(doneMutex);
        done = true;
    }

    queueCV.notify_all();

    for (auto& wt : wts) {
        wt.semaphore.signal();
    }

    availableWorkers.signal();
    if (dt.joinable()) dt.join();

    for (auto& wt : wts) {
        if (wt.ts.joinable()) {
            wt.ts.join();
        }
    }
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].semaphore.wait();

        {
            lock_guard<mutex> lock(doneMutex);
            if (done) break;
        }

        if (wts[id].thunk) {
            wts[id].thunk();
        }

        {
            lock_guard<mutex> lock(doneMutex);
            pendingTasks--;
            {
                lock_guard<mutex> lock(doneMutex);
                pendingTasks--;
                if (pendingTasks == 0) {
                    waitCV.notify_all();
                }
            }

        }

        {
            lock_guard<mutex> lock(wts[id].mtx);
            wts[id].available = true;
        }

        availableWorkers.signal();
    }
}

void ThreadPool::dispatcher() {
    while (true) {
        {
            unique_lock<mutex> lock(queueLock);
            queueCV.wait(lock, [this]() {
                return !tasks.empty() || done;
            });
            if (done && tasks.empty()) break;
        }

        availableWorkers.wait();

        function<void(void)> nextTask;
        {
            lock_guard<mutex> lock(queueLock);
            if (!tasks.empty()) {
                nextTask = move(tasks.front());
                tasks.pop();
            } else {
                availableWorkers.signal();
                continue;
            }
        }

        bool assigned = false;
        for (auto& wt : wts) {
            lock_guard<mutex> lg(wt.mtx);
            if (wt.available) {
                wt.available = false;
                wt.thunk = nextTask;
                wt.semaphore.signal();
                assigned = true;
                break;
            }
        }

        if (!assigned) {
            lock_guard<mutex> lock(queueLock);
            tasks.push(nextTask);
            availableWorkers.signal();
        }
    }

    for (auto& wt : wts) {
        wt.semaphore.signal();
    }
}

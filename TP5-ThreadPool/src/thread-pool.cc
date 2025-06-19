// thread-pool.cc
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

    {
        lock_guard<mutex> lock(waitLock);  // reemplaza doneMutex
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
    unique_lock<mutex> lock(waitLock);
    waitCV.wait(lock, [this]() {
        return pendingTasks == 0;
    });
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex> lock(waitLock);
        waitCV.wait(lock, [this] {
            return pendingTasks == 0 && tasks.empty();
        });
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
        wts[id].semaphore.wait();

        {
            lock_guard<mutex> lock(waitLock);
            if (done) break;  // ahora protegido
        }

        {
            lock_guard<mutex> lock(wts[id].mtx);
            if (wts[id].thunk) {
                wts[id].thunk();
                wts[id].thunk = nullptr;
            }
        }

        {
            lock_guard<mutex> lock(wts[id].mtx);
            wts[id].available = true;
        }

        {
            lock_guard<mutex> lock(waitLock);
            pendingTasks--;
            if (pendingTasks == 0) {
                waitCV.notify_all();
            }
        }

        availableWorkers.signal();
    }
}


void ThreadPool::dispatcher() {
    while (true) {
        {
            unique_lock<mutex> lock(queueLock);
            if (done && tasks.empty()) break;
            queueCV.wait(lock, [this] { return !tasks.empty() || done; });
            if (done && tasks.empty()) break;
            if (tasks.empty()) continue;
        }

        availableWorkers.wait();

        {
            lock_guard<mutex> lock(queueLock);
            for (auto& wt : wts) {
                lock_guard<mutex> wtlock(wt.mtx);
                if (wt.available && !tasks.empty()) {
                    wt.thunk = move(tasks.front());
                    tasks.pop();
                    wt.available = false;
                    wt.semaphore.signal();
                }
            }
        }
    }

    for (auto& wt : wts) {
        wt.semaphore.signal();
    }
}

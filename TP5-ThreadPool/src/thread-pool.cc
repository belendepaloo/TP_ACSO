// thread-pool.cc
#include "thread-pool.h"
#include <iostream>
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].id = i;
        wts[i].ready = false;
        wts[i].ts = thread([this, i]() { worker(i); });
    }

    dt = thread([this]() { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (!thunk) throw invalid_argument("Null task");

    {
        lock_guard<mutex> lock(waitMutex);
        if (done) throw runtime_error("Scheduling on a destroyed ThreadPool");
        pendingTasks++;
    }

    {
        lock_guard<mutex> lock(taskMutex);
        tasks.push(thunk);
    }

    taskAvailable.notify_one();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(waitMutex);
    waitCond.wait(lock, [this]() { return pendingTasks == 0; });
}

ThreadPool::~ThreadPool() {
    wait();

    {
        lock_guard<mutex> lock(waitMutex);
        done = true;
    }

    taskAvailable.notify_all();

    for (auto& w : wts) {
        {
            lock_guard<mutex> lock(w.mtx);
            w.ready = true;
        }
        w.cv.notify_one();
    }

    if (dt.joinable()) dt.join();

    for (auto& w : wts) {
        if (w.ts.joinable()) w.ts.join();
    }
}

void ThreadPool::worker(int id) {
    worker_t& w = wts[id];
    while (true) {
        unique_lock<mutex> lock(w.mtx);
        w.cv.wait(lock, [&]() { return w.ready || done; });

        if (done) break;

        function<void()> job;
        {
            lock_guard<mutex> tlock(w.mtx);
            job = w.task;
            w.task = nullptr;
            w.ready = false;
        }

        if (job) job();

        {
            lock_guard<mutex> lock(waitMutex);
            pendingTasks--;
            if (pendingTasks == 0) waitCond.notify_all();
        }

        {
            lock_guard<mutex> lock(wtQueueMutex);
            idleWorkers.push(id);
        }

        workerReady.notify_one();
    }
}

void ThreadPool::dispatcher() {
    while (true) {
        function<void()> job;
        {
            unique_lock<mutex> lock(taskMutex);
            taskAvailable.wait(lock, [this]() { return !tasks.empty() || done; });
            if (done && tasks.empty()) break;
            job = tasks.front();
            tasks.pop();
        }

        int id;
        {
            unique_lock<mutex> lock(wtQueueMutex);
            workerReady.wait(lock, [this]() { return !idleWorkers.empty(); });
            id = idleWorkers.front();
            idleWorkers.pop();
        }

        worker_t& w = wts[id];
        {
            lock_guard<mutex> lock(w.mtx);
            w.task = job;
            w.ready = true;
        }
        w.cv.notify_one();
    }

    for (auto& w : wts) {
        w.cv.notify_one();
    }
}

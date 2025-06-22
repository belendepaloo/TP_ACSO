#include "thread-pool.h"
#include <stdexcept> 

ThreadPool::ThreadPool(size_t numThreads): wts(numThreads), done(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].ts = thread([this, i] { worker(i); });
    }
    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (done.load()) {
        throw runtime_error("Cannot schedule after ThreadPool destruction");
    }
    if (!thunk) {
        throw invalid_argument("Scheduled task is null");
    }

    {
        lock_guard<mutex> lg(queueLock);
        tasks.push(thunk);
        pendingTasks++;
    }
    taskFree.notify_all();
}

void ThreadPool::dispatcher() {
    while (true) {
        function<void(void)> job;

        {
            unique_lock<mutex> lk(queueLock);
            taskFree.wait(lk, [this] { return done || !tasks.empty(); });

            if (done && tasks.empty()) break;

            job = tasks.front();
            tasks.pop();
        }

        unique_lock<mutex> wlk(workersLock);
        workerFree.wait(wlk, [this] {
            for (auto& w : wts)
                if (w.available.load()) return true;
            return false;
        });

        for (size_t i = 0; i < wts.size(); ++i) {
            if (wts[i].available.exchange(false)) {
                {
                    lock_guard<mutex> lock(queueLock);
                    wts[i].task = job;
                }
                wts[i].ready.signal();
                break;
            }
        }
    }
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].ready.wait();

        function<void()> taskToRun = nullptr;

        {
            lock_guard<mutex> lock(queueLock);
            taskToRun = wts[id].task;
            wts[id].task = nullptr;
        }

        if (done && taskToRun == nullptr) break;

        if (taskToRun) {
            taskToRun();
            {
                lock_guard<mutex> lock(queueLock);
                pendingTasks--;
                if (pendingTasks == 0) {
                    allDone.notify_all();
                }
            }
        }

        wts[id].available.store(true);
        workerFree.notify_all();
    }
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(queueLock);
    allDone.wait(lock, [this] { return pendingTasks == 0; });
}

ThreadPool::~ThreadPool() {
    wait();
    done = true;

    taskFree.notify_all();
    if (dt.joinable()) dt.join();

    for (auto& w : wts) {
        {
            lock_guard<mutex> lock(queueLock);
            w.task = nullptr;
        }
        w.ready.signal();
    }

    for (auto& w : wts) {
        if (w.ts.joinable()) w.ts.join();
    }
}

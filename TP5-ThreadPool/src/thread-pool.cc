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
    if (destructionStarted.load()) {
        throw runtime_error("Schedule called after destruction started");
    }

    {
        lock_guard<mutex> lock(queueLock);
        if (done) return;
        tasks.push(thunk);
    }
    {
        lock_guard<mutex> waitLockGuard(waitLock);
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
    {
        unique_lock<mutex> lock(queueLock);
        waitCV.wait(lock, [this] {
            return pendingTasks == 0 && tasks.empty();
        });
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
        cerr << "[worker " << id << "] esperando tarea..." << endl;
        wts[id].semaphore.wait();
        cerr << "[worker " << id << "] tarea recibida." << endl;

        if (done) {
            cerr << "[worker " << id << "] finalizando." << endl;
            break;
        }

        {
            lock_guard<mutex> lock(wts[id].mtx);
            if (wts[id].thunk) {
                wts[id].thunk();
                wts[id].thunk = nullptr;
            }
        }

        {
            lock_guard<mutex> lock(queueLock);
            {
                lock_guard<mutex> wtlock(wts[id].mtx);
                wts[id].available = true;
            }
            {
                lock_guard<mutex> waitLockGuard(waitLock);
                pendingTasks--;
                if (pendingTasks == 0) {
                    waitCV.notify_all();
                }
            }
            availableWorkers.signal();
            cerr << "[worker " << id << "] terminó tarea, queda " << pendingTasks.load() << " pendientes." << endl;
        }
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

        cerr << "[dispatcher] esperando worker disponible..." << endl;
        availableWorkers.wait();
        cerr << "[dispatcher] worker disponible encontrado." << endl;

        {
            lock_guard<mutex> lock(queueLock);
            for (auto& wt : wts) {
                lock_guard<mutex> wtlock(wt.mtx);
                if (wt.available && !tasks.empty()) {
                    wt.thunk = move(tasks.front());
                    tasks.pop();
                    wt.available = false;
                    wt.semaphore.signal();
                    cerr << "[dispatcher] tarea asignada a worker " << wt.id << endl;
                }
            }
        }
    }

    for (auto& wt : wts) {
        wt.semaphore.signal();
    }
}

#include "thread-pool.h"

ThreadPool::ThreadPool(size_t numThreads)
    : wts(numThreads), done(false), availableWorkers(numThreads) {

    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].ts = thread([this, i] { worker(i); });
    }

    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (done.load()) {
        throw runtime_error("Cannot schedule after ThreadPool destruction");
    }

    {
        lock_guard<mutex> lg(queueLock);
        tasks.push(thunk);
        pendingTasks++;
    }
    taskAvailable.notify_all(); // Notificar al dispatcher
}


void ThreadPool::dispatcher() {
    while (true) {
        function<void(void)> job;
        {
            unique_lock<mutex> lk(queueLock);
            taskAvailable.wait(lk, [this] { return done || !tasks.empty(); });

            if (done && tasks.empty()) break;

            job = tasks.front();
            tasks.pop();
        }

        availableWorkers.wait(); // esperar a que haya un worker libre

        for (size_t i = 0; i < wts.size(); ++i) {
            if (wts[i].available.exchange(false)) {
                wts[i].task = job;
                wts[i].ready.signal();
                break;
            }
        }
    }
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].ready.wait();

        if (done && wts[id].task == nullptr) break;

        if (wts[id].task) {
            wts[id].task();
            wts[id].task = nullptr;
            pendingTasks--;
        }

        wts[id].available.store(true);
        availableWorkers.signal();
    }
}

void ThreadPool::wait() {
    while (pendingTasks > 0) {
        this_thread::yield();
    }
}

ThreadPool::~ThreadPool() {
    wait(); // esperar que terminen las tareas

    done = true;

    taskAvailable.notify_all(); // liberar dispatcher
    if (dt.joinable()) dt.join();

    // mandar señal nula a cada worker para que se apaguen
    for (auto& w : wts) {
        w.task = nullptr;
        w.ready.signal();
    }

    for (auto& w : wts) {
        if (w.ts.joinable()) w.ts.join();
    }
}

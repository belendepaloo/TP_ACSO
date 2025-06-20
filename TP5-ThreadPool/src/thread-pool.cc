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

        // Esperar a que haya un worker disponible
        unique_lock<mutex> wlk(workersLock);
        workerAvailable.wait(wlk, [this] {
            for (auto& w : wts)
                if (w.available.load()) return true;
            return false;
        });

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
        wts[id].ready.wait();  // Espera señal del dispatcher

        if (done && wts[id].task == nullptr) break;

        if (wts[id].task) {
            wts[id].task();  // Ejecuta la tarea
            wts[id].task = nullptr;

            {
                lock_guard<mutex> lock(queueLock);
                pendingTasks--;
                if (pendingTasks == 0) {
                    allDone.notify_all();  // Despierta a wait()
                }
            }
        }

        wts[id].available.store(true);  // Marca disponible
        workerAvailable.notify_all();   // Notifica al dispatcher
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

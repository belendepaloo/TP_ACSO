#include "thread-pool.h"
#include <iostream>
#include <stdexcept>

using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : 
    done(false),
    pendingTasks(0),
    wts(numThreads) {
    
    // Inicializar workers
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].id = i;
        wts[i].ts = thread([this, i] { worker(i); });
    }

    // Iniciar dispatcher
    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (!thunk) {
        throw invalid_argument("Cannot schedule nullptr function");
    }

    {
        lock_guard<mutex> lock(waitMutex);
        if (done) {
            throw runtime_error("Scheduling on destroyed ThreadPool");
        }
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
    waitCond.wait(lock, [this] { 
        return pendingTasks == 0 || done.load(); 
    });
}

ThreadPool::~ThreadPool() {
    // Marcar para terminación
    {
        lock_guard<mutex> lock(waitMutex);
        done = true;
    }

    // Notificar a todos los threads
    taskAvailable.notify_all();
    workerReady.notify_all();

    // Notificar a los workers
    for (auto& w : wts) {
        {
            lock_guard<mutex> lock(w.mtx);
            w.ready = true;
        }
        w.cv.notify_one();
    }

    // Esperar a que terminen los threads
    if (dt.joinable()) {
        dt.join();
    }

    for (auto& w : wts) {
        if (w.ts.joinable()) {
            w.ts.join();
        }
    }
}

void ThreadPool::worker(int id) {
    worker_t& w = wts[id];
    
    while (true) {
        // Reportarse como disponible
        {
            lock_guard<mutex> lock(wtQueueMutex);
            idleWorkers.push(id);
        }
        workerReady.notify_one();

        // Esperar trabajo
        unique_lock<mutex> lock(w.mtx);
        w.cv.wait(lock, [&w, this] { 
            return w.ready || done.load(); 
        });

        if (done) break;

        // Ejecutar tarea
        if (w.task) {
            w.task();
            w.task = nullptr;
        }

        // Actualizar contador
        {
            lock_guard<mutex> lock(waitMutex);
            pendingTasks--;
            if (pendingTasks == 0) {
                waitCond.notify_all();
            }
        }
    }
}

void ThreadPool::dispatcher() {
    while (true) {
        // Esperar tarea disponible
        function<void()> task;
        {
            unique_lock<mutex> lock(taskMutex);
            taskAvailable.wait(lock, [this] { 
                return !tasks.empty() || done.load(); 
            });

            if (done && tasks.empty()) break;
            if (tasks.empty()) continue;

            task = move(tasks.front());
            tasks.pop();
        }

        // Esperar worker disponible
        int workerId;
        {
            unique_lock<mutex> lock(wtQueueMutex);
            workerReady.wait(lock, [this] { 
                return !idleWorkers.empty() || done.load(); 
            });

            if (done) break;

            workerId = idleWorkers.front();
            idleWorkers.pop();
        }

        // Asignar tarea
        worker_t& w = wts[workerId];
        {
            lock_guard<mutex> lock(w.mtx);
            w.task = move(task);
            w.ready = true;
        }
        w.cv.notify_one();
    }
}
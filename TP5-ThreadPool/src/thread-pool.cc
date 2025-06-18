/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].ts = thread([this, i]() {
            worker(i);
        });
    }

    dt = thread([this]() {
        dispatcher();
    });
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].ready.wait();
        if (done) break;

        function<void()> task;
        {
            lock_guard<mutex> lg(wts[id].mtx);
            task = wts[id].thunk;
            wts[id].thunk = nullptr;
            wts[id].available = true;
        }

        if (task) {
            task();         // ejecuta tarea
            tasksDone++;    // marca como completada
        }
    }
}


void ThreadPool::dispatcher() {
    while (true) {
        tasksPending.wait(); // Espera hasta que haya al menos una tarea

        if (done) break; // si el ThreadPool está cerrándose, salir

        function<void(void)> task;

        // Extraer la tarea de la cola
        {
            lock_guard<mutex> lock(queueLock);
            if (!taskQueue.empty()) {
                task = taskQueue.front();
                taskQueue.pop();
            } else {
                continue;
            }
        }

        bool assigned = false;
        while (!assigned && !done) {
            for (size_t i = 0; i < wts.size(); ++i) {
            unique_lock<mutex> wlock(wts[i].mtx);
            if (wts[i].available) {
                wts[i].available = false;       // Reservar primero
                wts[i].thunk = task;            // Asignar tarea
                wts[i].ready.signal();          // Despertar worker
                assigned = true;
                break;
            }
        }

            if (!assigned) {
                this_thread::yield();
            }
        }
    }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
        if (!thunk) {
        throw invalid_argument("No se puede encolar una función nula.");
        }
        if (!active) {
        throw logic_error("No se puede llamar a schedule() sobre un ThreadPool destruido.");
    }
    {
        lock_guard<mutex> lock(queueLock);
        taskQueue.push(thunk);
        tasksTotal++;
    }

    tasksPending.signal();
}


void ThreadPool::wait() {
    while (true) {
        bool allWorkersIdle = true;
        {
            lock_guard<mutex> lock(queueLock);
            if (!taskQueue.empty()) {
                allWorkersIdle = false;
            }
        }

        // Verificar que todos los workers estén disponibles
        for (size_t i = 0; i < wts.size() && allWorkersIdle; ++i) {
            lock_guard<mutex> wlock(wts[i].mtx);
            if (!wts[i].available) {
                allWorkersIdle = false;
            }
        }

        if (allWorkersIdle) break;
        this_thread::yield();
    }
}

ThreadPool::~ThreadPool() {
    wait();       // Esperar a que se terminen todas las tareas

    done = true;  // Señalar que el pool se está cerrando

    // Despertar al dispatcher (por si está bloqueado en tasksPending.wait)
    tasksPending.signal();

    // Despertar a todos los workers (por si están esperando con ready.wait)
    for (auto& w : wts) {
        w.ready.signal();  // Lo va a hacer salir del loop
    }

    // Esperar a que todos los workers terminen
    for (auto& w : wts) {
        if (w.ts.joinable()) {
            w.ts.join();
        }
    }

    // Esperar al dispatcher
    if (dt.joinable()) {
        dt.join();
    }
    active = false;
}


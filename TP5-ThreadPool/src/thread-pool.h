#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "Semaphore.h"

using namespace std;

typedef struct worker {
    thread ts;                       // hilo del worker
    function<void(void)> thunk;     // tarea asignada
    bool available = true;          // estado del worker
    Semaphore semaphore{0};         // semáforo del worker
    mutex mtx;                      // protege 'thunk' y 'available'
    int id = -1;                    // id del worker
} worker_t;

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

private:
    void worker(int id);
    void dispatcher();

    vector<worker_t> wts;                  // workers
    thread dt;                             // dispatcher
    queue<function<void(void)>> tasks;     // cola de tareas

    mutex queueLock;                       // protege la cola de tareas
    condition_variable queueCV;           // para que el dispatcher espere por tareas

    Semaphore availableWorkers;           // semáforo de disponibilidad de workers

    mutex doneMutex;                      // protege 'done' y 'pendingTasks'
    condition_variable waitCV;            // para wait()
    bool done;                            // indica si el threadpool se está destruyendo
    int pendingTasks = 0;                 // tareas activas

    // evitar copia y asignación
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
};

#endif

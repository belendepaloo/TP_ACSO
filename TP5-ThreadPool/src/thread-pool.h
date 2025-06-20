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
    thread ts;
    Semaphore ready{0};               // señal para ejecutar el trabajo
    function<void(void)> task;        // tarea asignada
    atomic<bool> available{true};    // si está disponible
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

    thread dt;                              // hilo despachador
    vector<worker_t> wts;                   // workers
    queue<function<void(void)>> tasks;      // cola de tareas

    mutex queueLock;
    condition_variable_any taskAvailable;   // notificación al dispatcher

    atomic<bool> done;                      // shutdown flag
    Semaphore availableWorkers;             // cuenta de workers disponibles
    atomic<int> pendingTasks{0};            // tareas en ejecución
};

#endif

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
    thread ts;                       // Hilo del worker
    function<void(void)> thunk;     // Tarea asignada al worker
    bool available;                 // Indica si el worker está libre
    Semaphore semaphore{0};         // Para esperar señales del dispatcher
    int id;                         // ID del worker
    mutex mtx;                      // Protege 'thunk' y 'available'
} worker_t;

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

private:
    void worker(int id);            // Función ejecutada por cada worker
    void dispatcher();              // Función ejecutada por el dispatcher

    thread dt;                      // Hilo dispatcher
    vector<worker_t> wts;           // Vector de workers
    queue<function<void(void)>> tasks; // Cola de tareas
    mutex queueLock;               // Protege el acceso a la cola de tareas
    condition_variable_any queueCV;// Notifica al dispatcher sobre nuevas tareas
    Semaphore availableWorkers;    // Controla cuántos workers están libres
    condition_variable_any waitCV; // Para que wait() sepa cuándo terminar
    mutex waitLock;   
    bool done = false;
    int pendingTasks = 0;

    // Evita la copia y asignación
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif

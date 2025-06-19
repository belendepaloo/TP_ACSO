/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <queue>       // for queue
#include <condition_variable> // for condition_variable
#include "Semaphore.h" // for Semaphore
#include <atomic>


using namespace std;

/**
 * @brief Represents a worker in the thread pool.
 */
typedef struct worker {
    thread ts;                     // Thread handle
    function<void(void)> thunk;    // Task to execute
    bool available;                // Availability status
    Semaphore semaphore{0};        // Semaphore to signal work availability
    int id;                        // Worker ID
} worker_t;

class ThreadPool {
public:
    /**
     * Constructs a ThreadPool configured to spawn up to the specified
     * number of threads.
     */
    ThreadPool(size_t numThreads);

    /**
     * Schedules the provided thunk (which is something that can
     * be invoked as a zero-argument function without a return value)
     * to be executed by one of the ThreadPool's threads as soon as
     * all previously scheduled thunks have been handled.
     */
    void schedule(const function<void(void)>& thunk);

    /**
     * Blocks and waits until all previously scheduled thunks
     * have been executed in full.
     */
    void wait();

    /**
     * Waits for all previously scheduled thunks to execute, and then
     * properly brings down the ThreadPool and any resources tapped
     * over the course of its lifetime.
     */
    ~ThreadPool();
    
private:
    void worker(int id);
    void dispatcher();

    thread dt;                              // dispatcher thread handle
    vector<worker_t> wts;                   // worker thread handles
    queue<function<void(void)>> tasks;      // queue of tasks to execute
    bool done;                              // flag to indicate the pool is being destroyed
    mutex queueLock;                        // mutex to protect the queue of tasks
    condition_variable_any queueCV;         // condition variable for task queue
    Semaphore availableWorkers;             // semaphore for available workers
    atomic<int> pendingTasks{0};            // counter for pending tasks
    condition_variable_any waitCV;          // condition variable for wait()
};

#endif
#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>      
#include <functional>  
#include <thread>      
#include <vector>       
#include <queue>        
#include <mutex>        
#include <condition_variable> 
#include "Semaphore.h" 

using namespace std;


/**
 * @brief Represents a worker in the thread pool.
 * 
 * The `worker_t` struct contains information about a worker 
 * thread in the thread pool. Should be includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * (or condition variable) to signal when work is ready for the 
 * worker to process.
 */
typedef struct worker {
    thread ts;
    function<void(void)> thunk;
    Semaphore ready{0};     
    bool available = true;  
    mutex mtx;              
} worker_t;

class ThreadPool {
  public:
    ThreadPool(size_t numThreads);

    void schedule(const function<void(void)>& thunk);

    void wait();

    ~ThreadPool();
    
private:
    void dispatcher();               
    void worker(int id);            

    thread dt;                      
    vector<worker_t> wts;           

    queue<function<void(void)>> tasks;
    mutex queueLock;                

    Semaphore tasksAvailable{0};    
    Semaphore workersAvailable;    

    int runningTasks = 0;           
    mutex doneMutex;                
    condition_variable_any allDoneCond;

    bool done;                     

    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif

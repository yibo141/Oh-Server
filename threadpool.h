#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <cstdio>
#include <cassert>
#include <stdexcept>
#include <pthread.h>
#include "locker.h"

template<typename T>
class threadpool
{
public:
    threadpool(int thread_number = 4, int max_requests = 100000);
    ~threadpool();
    bool add(T *request);

private:
    static void *worker(void *arg);
    void run();

    int thread_number;              // 线程池中线程数量
    int max_requests;               // 请求队列中允许的最大请求数
    std::vector<thread_t> threads;  // 描述线程池的数组，大小为thread_number
    std::queue<T*> workqueue;       // 请求队列
    locker queue_locker;            // 保护请求队列的互斥锁
    semaphore queue_stat;           // 是否有任务需要处理
    bool is_stop;                   // 是否结束线程
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests)
{
    assert(thread_number > 0 && max_requests > 0);
    this->thread_number = thread_number;
    this->max_requests = max_requests;
    threads.resize(thread_number);
    is_stop = false;

    // 创建thread_number个线程，并将它们设置为detach
    for(int i = 0; i < thread_number; ++i) {
        if(pthread_create(&threads[i], nullptr, worker, this) != 0);
            throw std::runtime_error("pthread_create error");
        if(pthread_detach(threads[i]) != 0)
            throw std::runtime_error("pthread_detach error");
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    is_stop = true;
}

template<typename T>
bool threadpool<T>::add(T *request)
{
    queue_locker.lock();

    // 如果当前请求队列中请求的数量超过允许的最大请求数量，则添加失败
    if(workqueue.size() > max_requests) {
        queue_locker.unlock();
        return false;
    }
    workqueue.push(request);
    queue_stat.post();
    return true;
}

template<typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while(!is_stop) {
        queue_stat.wait();      // 等待任务
        queue_locker.lock();
        if(workqueue.empty()){
            queue_locker.unlock();
            continue;
        }
        T *request = workqueue.front();
        workqueue.pop();
        queue_locker.unlock();
        if(request == NULL)
            continue;
        request->process();
    }
}
            
#endif // THREADPOOL_H

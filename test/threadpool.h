#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <stdexcept>
#include <pthread.h>
#include "locker.h"

template<typename T>
class threadpool
{
public:
    threadpool(int thread_number = 4, int max_requests = 10000);
    ~threadpool();
    bool add(T *request);

private:
    static void *worker(void *arg);
    void run();

    int thread_number;              // 线程池中线程数量
    int max_requests;               // 请求队列中允许的最大请求数
    pthread_t *threads; // 描述线程池的数组，大小为thread_number
    std::list<T*> workqueue;       // 请求队列
    locker queue_locker;            // 保护请求队列的互斥锁
    semaphore queue_stat;           // 是否有任务需要处理
    bool is_stop;                   // 是否结束线程
};

template<typename T>
threadpool<T>::threadpool(int number, int requests)
{
    assert(number > 0 && requests > 0);
    this->thread_number = number;
    this->max_requests = requests;
    is_stop = false;
    threads = new pthread_t[thread_number];
    if(!threads)
        throw std::runtime_error("malloc error");

    // 创建thread_number个线程，并将它们设置为detach
    for(int i = 0; i < thread_number; ++i) {
        std::cout << "create the " << i << "th thread" << std::endl;
        if(pthread_create(threads + i, NULL, worker, this) != 0);
        {
            delete[] threads;
            throw std::runtime_error("pthread_create error");
        }
        if(pthread_detach(threads[i]) != 0)
        {
            delete[] threads;
            throw std::runtime_error("pthread_detach error");
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[] threads;
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
    workqueue.push_back(request);
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
        workqueue.pop_front();
        queue_locker.unlock();
        if(request == NULL)
            continue;
        request->process();
    }
}
            
#endif // THREADPOOL_H

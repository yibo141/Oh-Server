#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <iostream>
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
    // 将任务添加到线程池
    bool add(T* request);

private:
    // 线程的入口函数，必须是static的
    static void* worker(void* arg);

    // 真正工作的函数
    void run();

private:
    int _thread_number;         // 线程池中线程的数目，一般等于CPU核心数
    int _max_requests;          // 最多的排队的请求个数
    pthread_t* _threads;        // 保存线程ID的数组
    std::list<T*> _workqueue;   // 请求队列
    locker _queuelocker;        // 保护请求队列的互斥锁
    semaphore _queuestat;       // 用于指示请求队列状态的信号量
    bool _stop;                 // 用于指示线程池是否停止工作
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests): 
        _thread_number(thread_number), _max_requests(max_requests), _stop(false), _threads(NULL)
{
    assert((thread_number > 0) && (max_requests > 0));

    _threads = new pthread_t[_thread_number];
    if(! _threads)
    {
        throw std::runtime_error("malloc error");
    }

    // 创建thread_number个线程并将它们设置为分离的
    for (int i = 0; i < thread_number; ++i)
    {
        std::cout << "create the number " << i << " thread" << std::endl;
        if(pthread_create( _threads + i, NULL, worker, this ) != 0)
        {
            delete[] _threads;
            throw std::runtime_error("pthread_create error");
        }
        if(pthread_detach(_threads[i]))
        {
            delete[] _threads;
            throw std::runtime_error("pthread_detach error");
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[] _threads;
    _stop = true;
}

template<typename T>
bool threadpool<T>::add(T* request)
{
    _queuelocker.lock();
    if (_workqueue.size() > _max_requests)
    {
        _queuelocker.unlock();
        return false;
    }
    _workqueue.push_back(request);
    _queuelocker.unlock();
    _queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while (! _stop)
    {
        _queuestat.wait();
        _queuelocker.lock();
        if (_workqueue.empty())
        {
            _queuelocker.unlock();
            continue;
        }
        T* request = _workqueue.front();
        _workqueue.pop_front();
        _queuelocker.unlock();
        if (!request)
        {
            continue;
        }
        request->process();
    }
}

#endif

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
    bool add(T* request);

private:
    static void* worker(void* arg);
    void run();

private:
    int _thread_number;
    int _max_requests;
    pthread_t* _threads;
    std::list< T* > _workqueue;
    locker _queuelocker;
    semaphore _queuestat;
    bool _stop;
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

    for (int i = 0; i < thread_number; ++i)
    {
        std::cout << "create the " << i << "th thread" << std::endl;
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

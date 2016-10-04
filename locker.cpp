#include "locker.h"

semaphore::semaphore()
{
    if(sem_init(&_sem, 0, 0) != 0)
        throw std::runtime_error("sem_init error");
}

semaphore::~semaphore()
{
    sem_destroy(&_sem);
}

bool semaphore::wait()
{
    return sem_wait(&_sem) == 0;
}

bool semaphore::post()
{
    return sem_post(&_sem) == 0;
}

locker::locker()
{
    if(pthread_mutex_init(&_mutex, NULL) != 0)
        throw std::runtime_error("pthread_mutex_init error");
}

locker::~locker()
{
    pthread_mutex_destroy(&_mutex);
}

bool locker::lock()
{
    return pthread_mutex_lock(&_mutex) == 0;
}

bool locker::unlock()
{
    return pthread_mutex_unlock(&_mutex) == 0;
}

condition::condition()
{
    if(pthread_mutex_init(&_mutex, NULL) != 0)
        throw std::runtime_error("pthread_mutex_init error");
    if(pthread_cond_init(&_cond, NULL) != 0)
    {
        pthread_mutex_destroy(&_mutex);
        throw std::runtime_error("pthread_cond_init error");
    }
}

condition::~condition()
{
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}

bool condition::wait()
{
    int ret = 0;
    pthread_mutex_lock(&_mutex);
    ret = pthread_cond_wait(&_cond, &_mutex);
    pthread_mutex_unlock(&_mutex);
    return ret == 0;
}

bool condition::signal()
{
    return pthread_cond_signal(&_cond) == 0;
}

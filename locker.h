#ifndef LOCKER_H
#define LOCKER_H

#include <stdexcept>
#include <pthread.h>
#include <semaphore.h>

// 封装信号量的类
class semaphore
{
public:
    semaphore();   // 创建并初始化信号量
    ~semaphore();  // 销毁信号量
    bool wait();   // 等待信号量
    bool post();   // 增加信号量
private:
    sem_t _sem;
};

// 封装互斥锁的类
class locker
{
public:
    locker();      // 创建并初始化互斥锁
    ~locker();     // 销毁互斥锁
    bool lock();   // 获取互斥锁
    bool unlock(); // 释放互斥锁
private:
    pthread_mutex_t _mutex;
};

// 封装条件变量的类
class condition
{
public:
    condition();   // 创建并初始化条件变量
    ~condition();  // 销毁条件变量
    bool wait();   // 等待条件变量
    bool signal(); // 唤醒等待条件变量的线程
private:
    pthread_mutex_t _mutex;  // 保护条件变量的互斥锁
    pthread_cond_t _cond;
};

#endif // LOCKER_H

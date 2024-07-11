#ifndef _APP_SEMAPHORE_H
#define _APP_SEMAPHORE_H

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class semaphore
{
public:
    semaphore();
    ~semaphore();

public:
    void init(int count);
    void signal();
    void wait();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int count_;
};




#endif

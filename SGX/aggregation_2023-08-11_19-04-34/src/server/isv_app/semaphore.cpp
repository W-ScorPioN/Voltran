#include "semaphore.h"


semaphore::semaphore()
{
}

semaphore::~semaphore()
{
}

void semaphore::init(int count)
{
    count_ = count;
}

void semaphore::signal() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    cv_.notify_all();
}

void semaphore::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [=] { return count_ > 0; });
    --count_;
}

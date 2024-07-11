#ifndef _CLIENT_APP_AGG_TIMER_H
#define _CLIENT_APP_AGG_TIMER_H

#include <sys/time.h>

#include <string>

class agg_timer
{
public:
    agg_timer(/* args */);
    ~agg_timer();
public:
    void setTip(std::string tip);
    void start();
    void end();
    void printTimeStamp();

private:
    std::string m_tip;

    struct timeval tv_start;
    struct timeval tv_end;
};

#endif
#include "agg_timer.h"

#include <sys/time.h>
#include <unistd.h>

#include <iostream>


agg_timer::agg_timer(/* args */)
{
}

agg_timer::~agg_timer()
{
}

void agg_timer::setTip(std::string tip)
{
    m_tip = tip;
}

void agg_timer::start()
{
    gettimeofday(&tv_start, NULL);
}

void agg_timer::end()
{
     gettimeofday(&tv_end, NULL);
}

void agg_timer::printTimeStamp()
{
    long start_us = tv_start.tv_sec * 1000000 + tv_start.tv_usec;
    long end_us = tv_end.tv_sec * 1000000 + tv_end.tv_usec;
    std::cout << std::endl << std::endl << std::endl << std::endl 
              << " >>> agg_timer::printTimeStamp, tip:" << m_tip << ", end_us - start_us=" << end_us - start_us 
              << std::endl << std::endl << std::endl << std::endl;
}


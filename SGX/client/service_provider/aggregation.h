#ifndef _PROVIDER_AGGREGATION_H
#define _PROVIDER_AGGREGATION_H

#include "schedule_config.h"
#include "common.h"

#include <map>
#include <string>

class aggregation
{
public:
    aggregation(/* args */);
    ~aggregation();

private:
    int parsePort(std::string sgxID);
    std::string parseIP(std::string sgxID);
    bool send2sgx(std::string sgxID, request_data_header_t *pdata);

public:
    int tryAggregating();
private:
    schedule_config schConfig;

    int m_taskID;
    int m_totalRound;
    int m_lastRound;

    std::map<std::string, sp_db_item_t> sgxID2spDB;
};


#endif

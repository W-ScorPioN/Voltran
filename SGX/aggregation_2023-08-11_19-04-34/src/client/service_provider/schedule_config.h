#ifndef _SCHEDULE_CONFIG_H
#define _SCHEDULE_CONFIG_H

#include <set>
#include <vector>
#include <map>
#include <string>

class schedule_config
{
    // struct dispath {
    //     std::vector<std::string> clientIP;
    //     std::string sgxIP;
    // };

private:
    /* data */
public:
    schedule_config();
    ~schedule_config();

    bool readConfig(std::string clientID);
    int getTaskID();
    int getTotalRound();
    int getCurrentRound();
    int getClientNumber();
    std::map<std::string, std::set<int> > getDispath();

private:
    int taskID;
    int totalRound;
    int currentRound;
    int clientNumber;
    
    std::map<std::string, std::set<int> > dispath;   // sgx的ip:端口 -- key数组
};



#endif
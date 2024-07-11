#ifndef _SCHEDULE_CONFIG_H
#define _SCHEDULE_CONFIG_H

#include <set>
#include <vector>
#include <map>
#include <string>
#include <mutex>          // std::mutex

#include <iostream>
#include <sstream>

struct SgxIDComp {
    bool operator() (const std::string& lsgxID, const std::string& rsgxID) const {
        char delim = ':';

        std::istringstream liss(lsgxID);
        std::string lstrIP;
        std::getline(liss, lstrIP, delim);
        std::string lstrPort;
        std::getline(liss, lstrPort, delim);

        std::istringstream riss(rsgxID);
        std::string rstrIP;
        std::getline(riss, rstrIP, delim);
        std::string rstrPort;
        std::getline(riss, rstrPort, delim);

        unsigned int lIPAddress = 0;
	    int lTmpIP[4] = {0};
        sscanf(lstrIP.c_str(), "%d.%d.%d.%d", &lTmpIP[0], &lTmpIP[1], &lTmpIP[2], &lTmpIP[3]);
        for(int i = 0; i < 4; i++)
        {
            lIPAddress += (lTmpIP[i] << (24 - (i * 8))  & 0xFFFFFFFF);
        }

        unsigned int rIPAddress = 0;
	    int rTmpIP[4] = {0};
        sscanf(rstrIP.c_str(), "%d.%d.%d.%d", &rTmpIP[0], &rTmpIP[1], &rTmpIP[2], &rTmpIP[3]);
        for(int i = 0; i < 4; i++)
        {
            rIPAddress += (rTmpIP[i] << (24 - (i * 8))  & 0xFFFFFFFF);
        }

        if(lIPAddress != rIPAddress)
        {
            return lIPAddress < rIPAddress;
        }

        return std::atoi(lstrPort.c_str()) < std::atoi(rstrPort.c_str());
	}
};

class schedule_config
{
private:
    schedule_config();
    ~schedule_config();
    static schedule_config instance;
public:
	static schedule_config* getInstance();
public:
    bool readConfig(std::string sgxID);
    int getTaskID();
    int getTotalRound();
    int getCurrentRound();
    int getIndex();
    std::set<std::string, SgxIDComp> getAllSgx();
    void getSendPack(std::map<int, std::set<int>> &send_pack);

private:
    std::mutex m_mutex;

    std::string m_sgxID;

    int m_taskID;
    int m_totalRound;
    int m_currentRound;

    int m_index;
    std::map<int, std::set<int>> m_send_pack;

    std::set<int> m_keys;
    std::set<std::string, SgxIDComp> m_allSgxID;     // 参与聚合的所有sgx的IP：端口
};



#endif
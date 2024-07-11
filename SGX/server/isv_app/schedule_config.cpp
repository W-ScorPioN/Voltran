
#include "schedule_config.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

schedule_config schedule_config::instance;

schedule_config::schedule_config()
{
    m_taskID = -1;
    m_totalRound = -1;
    m_currentRound = -1;
}

schedule_config::~schedule_config()
{
}

schedule_config* schedule_config::getInstance()
{
    return &instance;
}

bool schedule_config::readConfig(std::string sgxID)
{
    std::unique_lock<std::mutex> lck (m_mutex);

    m_sgxID = sgxID;

    std::ifstream f("schedule.json");
    json jData = json::parse(f);

    m_taskID = jData.at("taskID").get<int>();
    m_totalRound = jData.at("totalRound").get<int>();
    m_currentRound = jData.at("currentRound").get<int>();

    json jDistributionArray = jData.at("distribution");
    for (json::iterator it1 = jDistributionArray.begin(); it1 != jDistributionArray.end(); ++it1) {
        std::set<std::string, SgxIDComp> tmpAllSgxIP;
        bool flg = false;

        json jDispathArray = it1.value().at("dispath");
        for(json::iterator it2 = jDispathArray.begin(); it2 != jDispathArray.end(); ++it2)
        {
            json jDispath = it2.value();
            std::string tmpSgxIP = jDispath.at("sgxIP").get<std::string>();
            tmpAllSgxIP.insert(tmpSgxIP);

            if(tmpSgxIP != sgxID) 
            {
                continue;
            }

            flg = true;
        }
        if(!flg)
        {
            continue;
        }
        m_index = it1.value().at("index").get<int>();
        m_keys = it1.value().at("key").get<std::set<int>>();
        m_allSgxID = tmpAllSgxIP;

        json jSendPackArray = it1.value().at("sendPack");
        for(json::iterator itSendPack = jSendPackArray.begin(); itSendPack != jSendPackArray.end(); ++itSendPack)
        {
            int iIndex = itSendPack.value().at("index").get<int>();
            m_send_pack[iIndex] = itSendPack.value().at("key").get<std::set<int>>();
        }
    }
    for(std::set<std::string>::iterator itSgx = m_allSgxID.begin(); itSgx != m_allSgxID.end(); ++itSgx)
    {
        std::cout << *itSgx << ", ";
    }
    std::cout << std::endl;

    return true;
}

std::set<std::string, SgxIDComp> schedule_config::getAllSgx()
{
    return m_allSgxID;
}

void schedule_config::getSendPack(std::map<int, std::set<int>> &send_pack)
{
    std::unique_lock<std::mutex> lck (m_mutex);

    for(std::map<int, std::set<int>>::iterator itMapSendPack = m_send_pack.begin(); itMapSendPack != m_send_pack.end(); ++itMapSendPack)
    {
        std::set<int> tmpSet;
        std::set<int> keys = itMapSendPack->second;
        for(std::set<int>::iterator itSetKeys = keys.begin(); itSetKeys != keys.end(); ++itSetKeys)
        {
            tmpSet.insert(*itSetKeys);
        }
        send_pack[itMapSendPack->first] = tmpSet;
    }
}

int schedule_config::getTaskID()
{
    return m_taskID;
}

int schedule_config::getTotalRound()
{
    return m_totalRound;
}

int schedule_config::getCurrentRound()
{
    return m_currentRound;
}

int schedule_config::getIndex()
{
    return m_index;
}



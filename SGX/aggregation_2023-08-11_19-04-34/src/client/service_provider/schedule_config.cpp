
#include "schedule_config.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

schedule_config::schedule_config()
{
    taskID = -1;
    totalRound = -1;
    currentRound = -1;
    clientNumber = -1;
}

schedule_config::~schedule_config()
{
}

bool schedule_config::readConfig(std::string clientID)
{
    std::ifstream f("schedule.json");
    json jData = json::parse(f);

    taskID = jData.at("taskID").get<int>();
    totalRound = jData.at("totalRound").get<int>();
    currentRound = jData.at("currentRound").get<int>();
    clientNumber = jData.at("clientNumber").get<int>();

    json jDistributionArray = jData.at("distribution");
    for (json::iterator it1 = jDistributionArray.begin(); it1 != jDistributionArray.end(); ++it1) {
        std::set<int> keys = it1.value().at("key").get<std::set<int>>();

        json jDispathArray = it1.value().at("dispath");
        for(json::iterator it2 = jDispathArray.begin(); it2 != jDispathArray.end(); ++it2)
        {
            json jDispath = it2.value();

            std::vector<std::string> vecClient = jDispath.at("clientID").get<std::vector<std::string>>();
            std::vector<std::string>::iterator itExist = find(vecClient.begin(), vecClient.end(), clientID);
            if (itExist == vecClient.end())
            {
                continue;
            }

            std::string sgxIP = jDispath.at("sgxIP").get<std::string>();
            std::cout << std::endl << "[inner]sgxIP:" << sgxIP << std::endl;

            if(dispath.find(sgxIP) == dispath.end())
            {
                dispath[sgxIP] = keys;
                continue;
            }

            for(std::set<int>::iterator itSet = keys.begin(); itSet != keys.end(); ++itSet)
            {
                dispath[sgxIP].insert(*itSet);
            }
        }
    }

    return true;
}

int schedule_config::getTaskID()
{
    return taskID;
}

int schedule_config::getTotalRound()
{
    return totalRound;
}

int schedule_config::getCurrentRound()
{
    return currentRound;
}

int schedule_config::getClientNumber()
{
    return clientNumber;
}

std::map<std::string, std::set<int>> schedule_config::getDispath()
{
    return dispath;
}


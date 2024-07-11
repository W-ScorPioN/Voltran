#include "server_config.h"

#include <string>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

server_config server_config::instance;

server_config::server_config()
{
}

server_config::~server_config()
{
}

server_config* server_config::getInstance()
{
    return &instance;
}

bool server_config::readConfig(std::string appCfgFileName)
{
    // std::ifstream f("application.json");
    std::ifstream f(appCfgFileName);
    json jConfig = json::parse(f);

    m_ipAddr = jConfig.at("ip").get<std::string>();
    m_port = jConfig.at("port").get<int>();

    m_parallelAggNumber = jConfig.at("parallelAggNumber").get<int>();
    m_tcsNum = jConfig.at("tcsNum").get<int>();

    m_chainServerUrl = jConfig.at("chainServer").get<std::string>();

    return true;
}

std::string server_config::getSgxID()
{
    return m_ipAddr + ":" + std::to_string(m_port);
}

int server_config::getPort()
{
    return m_port;
}

int server_config::getParallelAggNumber()
{
    return m_parallelAggNumber;
}

int server_config::getTcsNum()
{
    return m_tcsNum;
}

std::string server_config::getChainServerUrl()
{
    return m_chainServerUrl;
}




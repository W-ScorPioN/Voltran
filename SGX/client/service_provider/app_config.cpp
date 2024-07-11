#include "app_config.h"

#include <string>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

app_config app_config::instance;

app_config::app_config()
{
}

app_config::~app_config()
{
}

app_config* app_config::getInstance()
{
    return &instance;
}

bool app_config::readConfig(std::string clientID)
{
    m_clientID = clientID;

    std::cout << std::endl << "[app_config::readConfig]clientID: " << m_clientID << std::endl << std::endl;

    return true;
}

std::string app_config::getClientID()
{
    return m_clientID;
}




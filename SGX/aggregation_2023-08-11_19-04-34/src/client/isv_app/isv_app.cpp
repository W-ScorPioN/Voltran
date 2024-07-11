
#include "remote_attestation.h"

#include <iostream>
#include "schedule_config.h"
#include "app_config.h"
#include "aggregation.h"
#include "ra_client.h"

bool init(std::string clientID)
{
    return app_config::getInstance()->readConfig(clientID);
}

int main(int argc, char* argv[])
{
    std::string appCfgFileName = "application.json";

    std::string strClientID = "0";

    if (argc > 1)
    {
        strClientID = argv[1];
    }
    std::cout << " >>> main, clientID:" << strClientID << std::endl;

    if(!init(strClientID))
    {
        std::cout << " >>> init failed. application config file:" << appCfgFileName << std::endl;
        return -1;
    }

    aggregation agg;
    agg.tryAggregating();
}


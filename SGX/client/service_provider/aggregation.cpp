#include "aggregation.h"

#include "app_config.h"
#include "learning_data.h"
#include "ra_client.h"
#include "remote_attestation.h"

#include "agg_timer.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <sstream>

aggregation::aggregation()
{
}

aggregation::~aggregation()
{
}

int aggregation::tryAggregating()
{
    std::string clientID = app_config::getInstance()->getClientID();

    schedule_config scheduleConfig;
    scheduleConfig.readConfig(clientID);

    m_taskID = scheduleConfig.getTaskID();
    m_totalRound = scheduleConfig.getTotalRound();
    m_lastRound = -1;

    learning_data learningData;
    learningData.setDirPath("./wh");

    while(true)
    {
        if(scheduleConfig.getTaskID() != m_taskID)
        {
            break;
        }

        int currentRound = scheduleConfig.getCurrentRound();

        if(m_lastRound == currentRound)
        {
            usleep(3000000);

            scheduleConfig.readConfig(clientID);
            continue;
        }
        m_lastRound = currentRound;

        learningData.parseAllFileName(std::stoi(clientID), currentRound);

        std::map<std::string, std::set<int>> dispath = scheduleConfig.getDispath();
        std::map<std::string, std::set<int>>::iterator it;
        for(it = dispath.begin(); it != dispath.end(); ++it)
        {
            std::set<int> keys = it->second;

            request_data_header_t * p_data = learningData.getRoundAndKeyData(scheduleConfig.getCurrentRound(), app_config::getInstance()->getClientID(), keys);
            if(NULL == p_data)
            {
                continue;
            }

            if(!send2sgx(it->first, p_data))
            {
                std::cout << " send data to sgx[" << it->first << "] failed." << std::endl;
                return -1;
            }
        }

        break;

        scheduleConfig.readConfig(clientID);
    }
}

int aggregation::parsePort(std::string sgxID)
{
    char delim = ':';
    std::istringstream iss(sgxID);

    std::string strIP;
    std::getline(iss, strIP, delim);
    
    std::string strPort;
    std::getline(iss, strPort, delim);

    return std::stoi(strPort);
}

std::string aggregation::parseIP(std::string sgxID)
{
    char delim = ':';
    std::istringstream iss(sgxID);
    std::string strIP;
    std::getline(iss, strIP, delim);
    
    return strIP;
}

bool aggregation::send2sgx(std::string sgxID, request_data_header_t *pdata)
{
    std::string sgxIP = parseIP(sgxID);
    int port = parsePort(sgxID);

    sgx_client client;
    if(-1 == client.connet(sgxIP.c_str(), port))
    {
        std::cout << " >>> aggregation::send2sgx, connect sgx server failed, sgxIP:" << sgxIP << ", port:" << port << std::endl << std::endl;
        return false;
    }

    agg_timer ra_timer;
    ra_timer.setTip("remote_attestation");
    ra_timer.start();

    remote_attestation ra;
    ra.attest(client);

    ra_timer.end();
    ra_timer.printTimeStamp();

    ra_samp_request_header_t *p_resp_msg = NULL;
    int ret = sp_ra_proc_feature_req(pdata, pdata->size, &p_resp_msg);
    if(0 != ret) {
        std::cout << "sp_ra_proc_feature_req failed. ret=" << ret << std::endl;
        return ret;
    }

    sp_aes_gcm_data_t *p_secret = (sp_aes_gcm_data_t *)p_resp_msg->body;

    agg_timer send_to_server_timer;
    send_to_server_timer.setTip("send2sgx");
    send_to_server_timer.start();

    client.SendSampRequestToServer(p_resp_msg);
    
    send_to_server_timer.end();
    send_to_server_timer.printTimeStamp();

    client.closeServerFd();

    return true;
}



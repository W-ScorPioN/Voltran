#include "aggregation_data.h"
#include "jute.h"

#include <string>

aggregation_data::aggregation_data()
{
}

aggregation_data::~aggregation_data()
{
}

void aggregation_data::resetEnv()
{
    for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin(); it != m_wt1_client.end(); ++it)
    {
        free((void *)it->second);
    }
    m_wt1_client.clear();

    for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_sgx.begin(); it != m_wt1_sgx.end(); ++it)
    {
        free((void *)it->second);
    }
    m_wt1_sgx.clear();

    m_keys.clear();
    m_allClients.clear();
    m_allSgxID.clear();
    m_clients_received.clear();
    m_sgxID_received.clear();

    m_taskID = "";
    m_totalRound = -1;
    m_currentRound = -1;
    m_clientNumber = -1;
}

void aggregation_data::parseSchedule(std::string scheduleCfg, std::string sgxID)
{
    m_sgxID = sgxID;

    jute::jValue jConfig = jute::parser::parse(scheduleCfg);

    m_taskID = jConfig["taskID"].as_string();
    m_totalRound = jConfig["totalRound"].as_int();
    m_currentRound = jConfig["currentRound"].as_int();
    m_clientNumber = jConfig["clientNumber"].as_int();

    jute::jValue jDistributionArray = jConfig["distribution"];
    int distributionSize = jDistributionArray.size();
    for(int i = 0; i < distributionSize; ++i)
    {
        std::set<int> tmpKeys;

        jute::jValue jKeys = jDistributionArray[i]["key"];
        int keySize = jKeys.size();
        for(int j = 0; j < keySize; ++j)
        {
            tmpKeys.insert(jKeys[j].as_int());
        }

        std::set<std::string> tmpAllSgxIP;

        jute::jValue jDispathArray = jDistributionArray[i]["dispath"];

        bool flg = false;
        int dispathArraySize = jDispathArray.size();
        for(int k = 0; k < dispathArraySize; k++)
        {
            std::string tmpSgxIP = jDispathArray[k]["sgxIP"].as_string();
            tmpAllSgxIP.insert(tmpSgxIP);

            if(tmpSgxIP != sgxID) 
            {
                continue;
            }

            flg = true;
            m_index = jDistributionArray[i]["index"].as_int();

            jute::jValue jClientIDArrary = jDispathArray[k]["clientID"];
            int clientIDArraySize = jClientIDArrary.size();
            for(int m = 0; m < clientIDArraySize; m++)
            {
                m_allClients.insert(jClientIDArrary[m].as_string());
            }
        }

        if(flg)
        {
            m_keys = tmpKeys;
            m_allSgxID = tmpAllSgxIP;
        }
    }
}

sgx_status_t aggregation_data::aggregate_client(request_data_header_t *p_data)
{
    char clientID[CLIENT_ID_SIZE];
    memset(clientID, 0, CLIENT_ID_SIZE);
    memcpy(clientID, p_data->clientID, CLIENT_ID_SIZE);
    if(m_clients_received.find(clientID) != m_clients_received.end())
    {
        return SGX_SUCCESS;
    }

    if(m_allClients.find(clientID) == m_allClients.end())
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    sgx_status_t ret;

    if(SGX_SUCCESS == ret)
    {
        m_clients_received.insert(clientID);
    }

    if(isAggregatingClientFinish())
    {
        m_sgxID_received.insert(m_sgxID);
    }

    return ret;
    // return SGX_SUCCESS;
}

bool aggregation_data::isAggregatingClientFinish()
{
    return m_allClients.size() == m_clients_received.size();
}

bool aggregation_data::isAggregatingSgxFinish()
{
    return m_allSgxID.size() == m_sgxID_received.size();
}

sgx_status_t aggregation_data::encrypt(
    const sgx_aes_gcm_128bit_key_t *aes_key,
    const sgx_ec256_private_t *priv_key,
    uint8_t* p_wt1, 
    uint32_t out_size_wt1, 
    uint8_t* p_wt1_mac,
    uint32_t out_size_wt1_mac,
    uint8_t* p_sign, 
    uint32_t out_size_sign)
{
    // int32_t totalModelSize = sizeof(double) + sizeof(double);
    // for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin(); it != m_wt1_client.end(); ++it)
    // {
    //     totalModelSize = sizeof(double) * 2 + totalModelSize + it->second->size;
    // }

    // uint8_t * p_model = (uint8_t *)malloc(totalModelSize);
    // double * p_tmp = (double *)p_model;
    // std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin();
    // if(it != m_wt1_client.end())
    // {
    //     *p_tmp = it->second->featureInfo.round;
    //     p_tmp++;
    //     *p_tmp = m_wt1_client.size();
    //     p_tmp++;
    // }
    // for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin(); it != m_wt1_client.end(); ++it)
    // {
    //     *p_tmp = it->second->featureInfo.featureSerialNumber;
    //     p_tmp++;
    //     *p_tmp = it->second->elementNumber;
    //     p_tmp++;
    //     memcpy(p_tmp, it->second->body, it->second->size);

    //     uint32_t clientCount = it->second->featureInfo.clientCount;
        
    //     std::map<uint32_t, feature_data_t *>::iterator itSgx = m_wt1_sgx.find(it->second->featureInfo.featureSerialNumber);
    //     if(itSgx != m_wt1_sgx.end())
    //     {
    //         uint32_t elementNumber = it->second->elementNumber;
    //         clientCount = clientCount + itSgx->second->featureInfo.clientCount;
    //         double *p_sgx_ele = (double *)itSgx->second->body;
    //         for(uint32_t i = 0; i < elementNumber; i++) {
    //             p_tmp[i] = (p_tmp[i] + p_sgx_ele[i]);
    //         }
    //     }
    //     for(uint32_t i = 0; i < it->second->elementNumber; i++) {
    //             p_tmp[i] = p_tmp[i] / clientCount;
    //     }
    //     p_tmp = p_tmp + it->second->elementNumber;
    // }

    uint8_t aes_iv[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00
    };
    sgx_status_t ret = sgx_rijndael128GCM_encrypt(aes_key,
                                    data,
                                    totalModelSize,
                                    (uint8_t *)p_wt1,
                                    &aes_iv[0],
                                    12,
                                    NULL,
                                    0,
                                    (sgx_aes_gcm_128bit_tag_t *)p_wt1_mac);
    free(p_model);

    if(SGX_SUCCESS != ret)
    {
        return ret;
    }

    sgx_ecc_state_handle_t state_handle;
    ret = sgx_ecc256_open_context(&state_handle);
    if(SGX_SUCCESS != ret)
    {
        return ret;
    }

    ret = sgx_ecdsa_sign(p_wt1, totalModelSize, priv_key, (sgx_ec256_signature_t *)p_sign, state_handle);
    if(SGX_SUCCESS != ret)
    {
        return ret;
    }
    (void)sgx_ecc256_close_context(state_handle);

    return SGX_SUCCESS;
}

// sgx_status_t aggregation_data::get_wt1_and_sign_size(
//     uint32_t *p_taskID,
//     uint32_t *p_round,
//     uint32_t *p_wt1_size,
//     uint32_t *p_wt1_mac_size,
//     uint32_t *p_signWt1_size)
// {
//     std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin();
//     if(it != m_wt1_client.end())
//     {
//         *p_taskID = 1234;
//         *p_round = it->second->featureInfo.round;
//     }

//     // round -- ke的个数 -- key编号- key元素个数 -- key元素 -- key编号- key元素个数 -- key元素 ...
//     int32_t totalModelSize = sizeof(double) + sizeof(double);
//     for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin(); it != m_wt1_client.end(); ++it)
//     {
//         totalModelSize = sizeof(double) * 2 + totalModelSize + it->second->size;
//     }
//     *p_wt1_size = totalModelSize;
//     *p_wt1_mac_size = sizeof(sgx_aes_gcm_128bit_tag_t);
//     *p_signWt1_size = sizeof(sgx_ec256_signature_t);

//     return SGX_SUCCESS;
// }

// uint32_t aggregation_data::calWt1SizeByKey(uint8_t *pkeys, uint32_t keySize)
// {
//     // round -- ke的个数 -- key编号- key元素个数 -- key元素 -- key编号- key元素个数 -- key元素 ...
//     uint32_t totalModelSize = sizeof(double) + sizeof(double);
//     uint32_t keyNumber = keySize / sizeof(uint32_t);
//     uint32_t *pkeys_u = (uint32_t *)pkeys;
//     for(uint32_t i = 0; i < keyNumber; i++)
//     {
//         std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.find(pkeys_u[i]);
//         if(it == m_wt1_client.end())
//         {
//             continue;
//         }
//         totalModelSize = sizeof(double) * 2 + totalModelSize + it->second->size;
//     }

//     return totalModelSize;
// }

sgx_status_t aggregation_data::get_wt1_and_sign_size_by_key(
    uint8_t* p_keys,
    uint32_t key_size,
    uint32_t *p_taskID,
    uint32_t *p_round,
    uint32_t *p_wt1_size,
    uint32_t *p_wt1_mac_size,
    uint32_t *p_signWt1_size)
{
    std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin();
    if(it != m_wt1_client.end())
    {
        *p_taskID = 1234;
        *p_round = it->second->featureInfo.round;
    }

    *p_wt1_size = calWt1SizeByKey(p_keys, key_size);
    *p_wt1_mac_size = sizeof(sgx_aes_gcm_128bit_tag_t);
    *p_signWt1_size = sizeof(sgx_ec256_signature_t);

    return SGX_SUCCESS;
}

// sgx_status_t aggregation_data::get_wt1_and_sign(
//     const sgx_aes_gcm_128bit_key_t *aes_key,
//     const sgx_ec256_private_t *priv_key,
//     uint8_t* p_wt1, 
//     uint32_t out_size_wt1, 
//     uint8_t* p_wt1_mac,
//     uint32_t out_size_wt1_mac,
//     uint8_t* p_sign, 
//     uint32_t out_size_sign)
// {
//     int32_t totalModelSize = sizeof(double) + sizeof(double);
//     for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin(); it != m_wt1_client.end(); ++it)
//     {
//         totalModelSize = sizeof(double) * 2 + totalModelSize + it->second->size;
//     }

//     uint8_t * p_model = (uint8_t *)malloc(totalModelSize);
//     double * p_tmp = (double *)p_model;
//     std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin();
//     if(it != m_wt1_client.end())
//     {
//         *p_tmp = it->second->featureInfo.round;
//         p_tmp++;
//         *p_tmp = m_wt1_client.size();
//         p_tmp++;
//     }
//     for(std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin(); it != m_wt1_client.end(); ++it)
//     {
//         *p_tmp = it->second->featureInfo.featureSerialNumber;
//         p_tmp++;
//         *p_tmp = it->second->elementNumber;
//         p_tmp++;
//         memcpy(p_tmp, it->second->body, it->second->size);

//         uint32_t clientCount = it->second->featureInfo.clientCount;
        
//         std::map<uint32_t, feature_data_t *>::iterator itSgx = m_wt1_sgx.find(it->second->featureInfo.featureSerialNumber);
//         if(itSgx != m_wt1_sgx.end())
//         {
//             uint32_t elementNumber = it->second->elementNumber;
//             clientCount = clientCount + itSgx->second->featureInfo.clientCount;
//             double *p_sgx_ele = (double *)itSgx->second->body;
//             for(uint32_t i = 0; i < elementNumber; i++) {
//                 p_tmp[i] = (p_tmp[i] + p_sgx_ele[i]);
//             }
//         }
//         for(uint32_t i = 0; i < it->second->elementNumber; i++) {
//                 p_tmp[i] = p_tmp[i] / clientCount;
//         }
//         p_tmp = p_tmp + it->second->elementNumber;
//     }

//     uint8_t aes_iv[12] = {
//         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
//         0x00, 0x00, 0x00, 0x00
//     };
//     sgx_status_t ret = sgx_rijndael128GCM_encrypt(aes_key,
//                                     p_model,
//                                     totalModelSize,
//                                     (uint8_t *)p_wt1,
//                                     &aes_iv[0],
//                                     12,
//                                     NULL,
//                                     0,
//                                     (sgx_aes_gcm_128bit_tag_t *)p_wt1_mac);
//     free(p_model);

//     if(SGX_SUCCESS != ret)
//     {
//         return ret;
//     }

//     sgx_ecc_state_handle_t state_handle;
//     ret = sgx_ecc256_open_context(&state_handle);
//     if(SGX_SUCCESS != ret)
//     {
//         return ret;
//     }

//     ret = sgx_ecdsa_sign(p_wt1, totalModelSize, priv_key, (sgx_ec256_signature_t *)p_sign, state_handle);
//     if(SGX_SUCCESS != ret)
//     {
//         return ret;
//     }
//     (void)sgx_ecc256_close_context(state_handle);

//     return SGX_SUCCESS;
// }

// sgx_status_t aggregation_data::get_wt1_and_sign_by_key(
//         const sgx_aes_gcm_128bit_key_t *aes_key,
//         const sgx_ec256_private_t *priv_key,
//         uint8_t* p_keys,
//         uint32_t key_size,
//         uint8_t* p_wt1,
//         uint32_t out_size_wt1,
//         uint8_t* p_wt1_mac,
//         uint32_t out_size_wt1_mac,
//         uint8_t* p_sign,
//         uint32_t out_size_sign)
// {
//     double * p_tmp = (double *)p_wt1;
//     std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.begin();
//     if(it != m_wt1_client.end())
//     {
//         *p_tmp = it->second->featureInfo.round;
//         p_tmp++;
//     }
//     uint32_t keyNumber = key_size / sizeof(uint32_t);
//     *p_tmp = keyNumber;
//     p_tmp++;
    
//     uint32_t *pkeys_u = (uint32_t *)p_keys;
//     for(uint32_t i = 0; i < keyNumber; i++)
//     {
//         std::map<uint32_t, feature_data_t *>::iterator it = m_wt1_client.find(pkeys_u[i]);
//         if(it == m_wt1_client.end())
//         {
//             continue;
//         }

//         *p_tmp = it->second->featureInfo.featureSerialNumber;
//         p_tmp++;
//         *p_tmp = it->second->elementNumber;
//         p_tmp++;
//         memcpy(p_tmp, it->second->body, it->second->size);

//         uint32_t clientCount = it->second->featureInfo.clientCount;
        
//         std::map<uint32_t, feature_data_t *>::iterator itSgx = m_wt1_sgx.find(it->second->featureInfo.featureSerialNumber);
//         if(itSgx != m_wt1_sgx.end())
//         {
//             uint32_t elementNumber = it->second->elementNumber;
//             clientCount = clientCount + itSgx->second->featureInfo.clientCount;
//             double *p_sgx_ele = (double *)itSgx->second->body;
//             for(uint32_t i = 0; i < elementNumber; i++) {
//                 p_tmp[i] = (p_tmp[i] + p_sgx_ele[i]);
//             }
//         }
//         for(uint32_t i = 0; i < it->second->elementNumber; i++) {
//                 p_tmp[i] = p_tmp[i] / clientCount;
//         }
//         p_tmp = p_tmp + it->second->elementNumber;


//     }
//     return SGX_SUCCESS;
// }




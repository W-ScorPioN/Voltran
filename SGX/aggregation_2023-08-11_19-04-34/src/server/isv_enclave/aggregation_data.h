#ifndef _SGX_AGGREGATION_DATA_H
#define _SGX_AGGREGATION_DATA_H

#include <set>
#include <string>
#include <map>

#include "request_data_header.h"
#include "sgx_error.h"

#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"

class aggregation_data
{
private:
    /* data */
public:
    aggregation_data();
    ~aggregation_data();

public:
    void resetEnv();
    void parseSchedule(std::string scheduleCfg, std::string sgxID);

    sgx_status_t aggregate_client(request_data_header_t *p_data);

    bool isAggregatingClientFinish();
    bool isAggregatingSgxFinish();

    sgx_status_t get_wt1_and_sign_size(
        uint32_t *p_taskID,
        uint32_t *p_round,
        uint32_t *p_wt1_size,
        uint32_t *p_wt1_mac_size,
        uint32_t *p_signWt1_size);

    sgx_status_t get_wt1_and_sign_size_by_key(
        uint8_t* p_keys,
        uint32_t key_size,
        uint32_t *p_taskID,
        uint32_t *p_round,
        uint32_t *p_wt1_size,
        uint32_t *p_wt1_mac_size,
        uint32_t *p_signWt1_size);
    
    sgx_status_t get_wt1_and_sign(
        const sgx_aes_gcm_128bit_key_t *aes_key,
        const sgx_ec256_private_t *priv_key,
        uint8_t* p_wt1,
        uint32_t out_size_wt1,
        uint8_t* p_wt1_mac,
        uint32_t out_size_wt1_mac,
        uint8_t* p_sign,
        uint32_t out_size_sign);
    
    sgx_status_t get_wt1_and_sign_by_key(
        const sgx_aes_gcm_128bit_key_t *aes_key,
        const sgx_ec256_private_t *priv_key,
        uint8_t* p_keys,
        uint32_t key_size,
        uint8_t* p_wt1,
        uint32_t out_size_wt1,
        uint8_t* p_wt1_mac,
        uint32_t out_size_wt1_mac,
        uint8_t* p_sign,
        uint32_t out_size_sign);
    
private:
    uint32_t calWt1SizeByKey(uint8_t *pkeys, uint32_t keySize);
private:
    std::map<uint32_t, feature_data_t *> m_wt1_client;      // 特征号 -- 该特征的数据（数组）
    std::map<uint32_t, feature_data_t *> m_wt1_sgx;    // 其他分片数据，特征号 -- 该特征的数据（数组）

    std::string m_sgxID;

    std::string m_taskID;
    int m_totalRound;
    int m_currentRound;
    int m_clientNumber;
    
    int m_index;

    std::set<int> m_keys;
    std::set<std::string> m_allClients;   // 本sgx分片的所有客户端
    std::set<std::string> m_allSgxID;     // 参与聚合的所有sgx的IP：端口

    std::set<std::string> m_clients_received;   // 已收到的客户端的数据 
    std::set<std::string> m_sgxID_received;     // 已收到的sgx分片的数据
};




#endif
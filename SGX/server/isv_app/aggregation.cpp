#include "aggregation.h"
#include <stdio.h>
#include <iostream>

#include "network_ra.h"
#include "remote_attestation_result.h"
#include "sgx_invoke_status.h"
#include "enclave_invoker.h"
#include "server_config.h"
#include "schedule_config.h"
#include "util.h"
#include "agg_timer.h"
#include "service_provider.h"

#include "isv_enclave_u.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static const sgx_aes_gcm_128bit_key_t g_aes_key = {
    0xBE, 0xE1, 0x34, 0x79, 0x41, 0x82, 0x7D, 0xAB,
    0xD8, 0xE8, 0xB1, 0x12, 0x6C, 0xDC, 0x6F, 0xD1
};

static const sgx_ec256_private_t g_priv_key = {
    {
        0xEB, 0xA7, 0xA9, 0x4E, 0xA8, 0x8B, 0xE6, 0x93, 
        0x86, 0x7C, 0x29, 0xED, 0x58, 0xEB, 0x9A, 0x6C, 
        0x94, 0x5D, 0x95, 0x90, 0x52, 0x24, 0xE4, 0x42, 
        0x47, 0x86, 0x17, 0xF6, 0x63, 0x2C, 0x35, 0xBE 
    }
};

static const uint8_t g_aes_iv[12] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00
};

aggregation::aggregation(long cfd, sgx_enclave_id_t enclaveID, sgx_ra_context_t context)
{
    this->client_sockfd = cfd;
    this->enclave_id = enclaveID;
    this->context = context;
}

aggregation::~aggregation()
{
}

int aggregation::aggregate_client(ra_samp_request_header_t *req, uint8_t* msk)
{
    FILE *OUTPUT = stdout;
    if(NULL == req) 
    {
        return -1;
    }
    sp_aes_gcm_data_t *p_secret = (sp_aes_gcm_data_t *)req->body;

    agg_timer aggTimer;
    aggTimer.setTip("aggregate_client");
    aggTimer.start();

    uint32_t isFinish;

    sgx_status_t ret = enclave_invoker::getInstance()->aggregating_client_t(
        enclave_id, 
        &status,
        context,
        p_secret->payload,
        p_secret->payload_size,
        p_secret->payload_tag,
        &isFinish,
        msk);
    
    aggTimer.end();
    aggTimer.printTimeStamp();

    // if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
    // {
    //     return ret;
    // }

    // ret = enclave_invoker::getInstance()->get_mult_sgx_agg_status_t(
    //     enclave_id, 
    //     &status,
    //     context,
    //     &isFinish);

    // if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
    // {
    //     return ret;
    // }

    // if(SGX_MULT_AGG_FINISH == isFinish)
    // {
    //     send_wt1_to_chain_loop();
    // }

    return 0;
}

int aggregation::send_wt1_to_chain_loop()
{
    std::map<int, std::set<int>> send_pack;
    schedule_config::getInstance()->getSendPack(send_pack);

    for(std::map<int, std::set<int>>::iterator itMapSendPack = send_pack.begin(); itMapSendPack != send_pack.end(); ++itMapSendPack)
    {
        std::set<int> keys = itMapSendPack->second;
        send_wt1_to_chain_by_pack(itMapSendPack->first, keys);
    }

    return 0;
}

int aggregation::send_wt1_to_chain_by_pack(int index, std::set<int> keys)
{
    FILE *OUTPUT = stdout;

    uint8_t * p_wt1_data = NULL;
    uint8_t * p_wt1_data_encrypt = NULL;
    uint8_t * p_wt1_mac = NULL;
    uint8_t * p_sign_wt1_data = NULL;

    char * p_hex_wt1_data = NULL;
    char * p_hex_wt1_mac_data = NULL;
    char * p_hex_wt1_sign_data = NULL;

    uint8_t * p_keys = NULL;

    do 
    {
        uint32_t taskID;
        uint32_t round;
        uint32_t wt1_size;
        uint32_t wt1_mac_size;
        uint32_t sign_wt1_size;

        uint32_t key_size = keys.size() * sizeof(uint32_t);
        p_keys = (uint8_t *)malloc(key_size);
        uint32_t *p_tmp = (uint32_t *)p_keys;
        uint32_t idx = 0;
        for(std::set<int>::iterator itSetKeys = keys.begin(); itSetKeys != keys.end(); ++itSetKeys)
        {
            p_tmp[idx] = *itSetKeys;
            idx++;
        }

        sgx_status_t ret = enclave_invoker::getInstance()->get_wt1_and_sign_size_by_key_t(
                                        enclave_id,
                                        &status,
                                        context,
                                        p_keys,
                                        key_size,
                                        &taskID,
                                        &round,
                                        &wt1_size,
                                        &wt1_mac_size,
                                        &sign_wt1_size);
        if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
        {
            break;
        }

        p_wt1_data = (uint8_t *)malloc(wt1_size + 1);
        p_wt1_data_encrypt = (uint8_t *)malloc(wt1_size + 1);
        p_wt1_mac = (uint8_t *)malloc(wt1_mac_size + 1);
        p_sign_wt1_data = (uint8_t *)malloc(sign_wt1_size + 1);

        if(NULL == p_wt1_data || NULL == p_wt1_mac || NULL == p_sign_wt1_data)
        {
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        agg_timer get_wt1_and_sign_Timer;
        get_wt1_and_sign_Timer.setTip("send_wt1_to_chain_by_pack, get_wt1_and_sign_by_key_t\n");
        get_wt1_and_sign_Timer.start();

        ret = enclave_invoker::getInstance()->get_wt1_and_sign_by_key_t(
                                                    enclave_id,
                                                    &status,
                                                    context,
                                                    p_keys,
                                                    key_size,
                                                    p_wt1_data,
                                                    wt1_size,
                                                    p_wt1_mac,
                                                    wt1_mac_size,
                                                    p_sign_wt1_data,
                                                    sign_wt1_size);
        if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
        {
            break;
        }

        int ret_encrypt = sp_proc_encrypt_data((uint8_t *)g_aes_key,
                            p_wt1_data,
                            wt1_size,
                            p_wt1_data_encrypt,
                            p_wt1_mac);
        if(ret_encrypt)
        {
            break;
        }

        int ret_sign = sp_proc_sign_data(
            p_wt1_data_encrypt,
            wt1_size,
            (uint8_t *)&g_priv_key,
            p_sign_wt1_data
        );

        get_wt1_and_sign_Timer.end();
        get_wt1_and_sign_Timer.printTimeStamp();
        
        if(ret_sign)
        {
            break;
        }

        p_hex_wt1_data = (char *)malloc( wt1_size * 2 + 1);
        p_hex_wt1_mac_data = (char *)malloc(wt1_mac_size * 2 + 1);
        p_hex_wt1_sign_data = (char *)malloc(sign_wt1_size * 2 + 1);
        if(NULL == p_hex_wt1_data || NULL == p_hex_wt1_mac_data || NULL == p_hex_wt1_sign_data)
        {
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }
        memset(p_hex_wt1_data, 0, wt1_size * 2 + 1);
        memset(p_hex_wt1_mac_data, 0, wt1_mac_size * 2 + 1);
        memset(p_hex_wt1_sign_data, 0, sign_wt1_size * 2 + 1);
        ByteToHexStr((unsigned char*)p_wt1_data_encrypt, p_hex_wt1_data, wt1_size);
        ByteToHexStr((unsigned char*)p_wt1_mac, p_hex_wt1_mac_data, wt1_mac_size);
        ByteToHexStr((unsigned char*)p_sign_wt1_data, p_hex_wt1_sign_data, sign_wt1_size);
        p_hex_wt1_data[wt1_size * 2] = '\0';
        p_hex_wt1_mac_data[wt1_mac_size * 2] = '\0';
        p_hex_wt1_sign_data[sign_wt1_size * 2] = '\0';

        std::string strJWt1 = p_hex_wt1_data;
        std::string strJWt1Mac = p_hex_wt1_mac_data;
        std::string strJWt1_sign = p_hex_wt1_sign_data;

        json j;
        // j["taskID"] = taskID;
        j["taskID"] = schedule_config::getInstance()->getTaskID();
        j["round"] = round;
        j["ciphertext"] = strJWt1;
        j["auth_tag"] = strJWt1Mac;
        j["sig"] = strJWt1_sign;
        j["index"] = index;
        std::string jstr = j.dump();

        sendWt1ToChain(jstr);
    } while(0);

    SAFE_FREE(p_wt1_data);
    SAFE_FREE(p_wt1_data_encrypt);
    SAFE_FREE(p_wt1_mac);
    SAFE_FREE(p_sign_wt1_data);
    SAFE_FREE(p_hex_wt1_data);
    SAFE_FREE(p_hex_wt1_mac_data);
    SAFE_FREE(p_hex_wt1_sign_data);
    SAFE_FREE(p_keys);

    return 0;
}

int aggregation::send_wt1_to_chain()
{
    FILE *OUTPUT = stdout;

    uint32_t taskID;
    uint32_t round;
    uint32_t wt1_size;
    uint32_t wt1_mac_size;
    uint32_t sign_wt1_size;
    sgx_status_t ret = enclave_invoker::getInstance()->get_wt1_and_sign_size_t(
                                    enclave_id,
                                    &status,
                                    context,
                                    &taskID,
                                    &round,
                                    &wt1_size,
                                    &wt1_mac_size,
                                    &sign_wt1_size);
    if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
    {
        return -1;
    }

    uint8_t * p_wt1_data = NULL;
    uint8_t * p_wt1_mac = NULL;
    uint8_t * p_sign_wt1_data = NULL;

    char * p_hex_wt1_data = NULL;
    char * p_hex_wt1_mac_data = NULL;
    char * p_hex_wt1_sign_data = NULL;

    do 
    {
        p_wt1_data = (uint8_t *)malloc(wt1_size + 1);
        p_wt1_mac = (uint8_t *)malloc(wt1_mac_size + 1);
        p_sign_wt1_data = (uint8_t *)malloc(sign_wt1_size + 1);
        if(NULL == p_wt1_data || NULL == p_wt1_mac || NULL == p_sign_wt1_data)
        {
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        agg_timer get_wt1_and_sign_Timer;
        get_wt1_and_sign_Timer.setTip("send_wt1_to_chain, get_wt1_and_sign");
        get_wt1_and_sign_Timer.start();

        ret = enclave_invoker::getInstance()->get_wt1_and_sign_t(
                                                    enclave_id,
                                                    &status,
                                                    context,
                                                    p_wt1_data,
                                                    wt1_size,
                                                    p_wt1_mac,
                                                    wt1_mac_size,
                                                    p_sign_wt1_data,
                                                    sign_wt1_size);
        get_wt1_and_sign_Timer.end();
        get_wt1_and_sign_Timer.printTimeStamp();

        if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
        {
            break;
        }

        p_hex_wt1_data = (char *)malloc( wt1_size * 2 + 1);
        p_hex_wt1_mac_data = (char *)malloc(wt1_mac_size * 2 + 1);
        p_hex_wt1_sign_data = (char *)malloc(sign_wt1_size * 2 + 1);
        if(NULL == p_hex_wt1_data || NULL == p_hex_wt1_mac_data || NULL == p_hex_wt1_sign_data)
        {
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }
        memset(p_hex_wt1_data, 0, wt1_size * 2 + 1);
        memset(p_hex_wt1_mac_data, 0, wt1_mac_size * 2 + 1);
        memset(p_hex_wt1_sign_data, 0, sign_wt1_size * 2 + 1);
        ByteToHexStr((unsigned char*)p_wt1_data, p_hex_wt1_data, wt1_size);
        ByteToHexStr((unsigned char*)p_wt1_mac, p_hex_wt1_mac_data, wt1_mac_size);
        ByteToHexStr((unsigned char*)p_sign_wt1_data, p_hex_wt1_sign_data, sign_wt1_size);
        p_hex_wt1_data[wt1_size * 2] = '\0';
        p_hex_wt1_mac_data[wt1_mac_size * 2] = '\0';
        p_hex_wt1_sign_data[sign_wt1_size * 2] = '\0';

        std::string strJWt1 = p_hex_wt1_data;
        std::string strJWt1Mac = p_hex_wt1_mac_data;
        std::string strJWt1_sign = p_hex_wt1_sign_data;

        json j;
        // j["taskID"] = taskID;
        j["taskID"] = schedule_config::getInstance()->getTaskID();
        j["round"] = round;
        j["ciphertext"] = strJWt1;
        j["auth_tag"] = strJWt1Mac;
        j["sig"] = strJWt1_sign;
        j["index"] = schedule_config::getInstance()->getIndex();
        std::string jstr = j.dump();

        sendWt1ToChain(jstr);
    } while(0);

    SAFE_FREE(p_wt1_data);
    SAFE_FREE(p_wt1_mac);
    SAFE_FREE(p_sign_wt1_data);
    SAFE_FREE(p_hex_wt1_data);
    SAFE_FREE(p_hex_wt1_mac_data);
    SAFE_FREE(p_hex_wt1_sign_data);

    return 0;
}

void aggregation::sendWt1ToChain(std::string jsonData)
{
    char * szJsonData = (char *)malloc(jsonData.size() + 1);
    if(NULL == szJsonData)
    {
        return;
    }
    memset(szJsonData, 0, jsonData.size() + 1);
    strcpy(szJsonData, jsonData.c_str());

    agg_timer sendToChainTimer;
    sendToChainTimer.setTip("sendWt1ToChain");
    sendToChainTimer.start();

    //1. 创建一个curl句柄
    CURL* curl = curl_easy_init();

    if(curl) {
        //3. 给该句柄设定一些参数 (封装一个http请求消息)
        // http://10.114.100.154:9001 wang
        std::string url = server_config::getInstance()->getChainServerUrl() + "/tee/uploadGlobalModel";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        //给当前curl变成post请求
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        // 设置http发送的内容类型为JSON
        curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);

        //给当前curl设置需要传递post数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szJsonData);

        //4. 将curl句柄 向远程服务器 提交请求 并得到一个返回值
        CURLcode res = curl_easy_perform(curl);  //阻塞等待服务器返回
        if(res != CURLE_OK) {
            printf("curl easy perform error res = %d\n", res);
            SAFE_FREE(szJsonData);
            curl_easy_cleanup(curl);
            return;
        }

        //5. 处理服务器返回数据

        //6. 清空 释放句柄内存空间
        curl_easy_cleanup(curl);
    }

    sendToChainTimer.end();
    sendToChainTimer.printTimeStamp();
    
    SAFE_FREE(szJsonData);
}

void aggregation::ByteToHexStr(const unsigned char* source, char* dest, uint32_t sourceLen)
{
    uint32_t i;
    unsigned char highByte, lowByte;
 
    for (i = 0; i < sourceLen; i++)
    {
        highByte = source[i] >> 4;
        lowByte = source[i] & 0x0f;
 
        highByte += 0x30;
 
        if (highByte > 0x39)
            dest[i * 2] = highByte + 0x07;
        else
            dest[i * 2] = highByte;
 
        lowByte += 0x30;
        if (lowByte > 0x39)
            dest[i * 2 + 1] = lowByte + 0x07;
        else
            dest[i * 2 + 1] = lowByte;
    }
    return;
}


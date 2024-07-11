#include <stdio.h>
#include <limits.h>
#include <unistd.h>
// Needed for definition of remote attestation messages.
#include "remote_attestation_result.h"

#include "isv_enclave_u.h"

// Needed to call untrusted key exchange library APIs, i.e. sgx_ra_proc_msg2.
#include "sgx_ukey_exchange.h"

// Needed to get service provider's information, in your real project, you will
// need to talk to real server.
#include "network_ra.h"

// Needed to create enclave and do ecall.
#include "sgx_urts.h"

// Needed to query extended epid group id.
#include "sgx_uae_epid.h"
#include "sgx_uae_quote_ex.h"

#include "service_provider.h"

#include "request_data_header.h"
#include "enclave_manager.h"
#include "remote_attestation.h"
#include "aggregation.h"
#include "schedule_config.h"
#include "server_config.h"
#include "router.h"
#include "enclave_invoker.h"


#ifndef SAFE_FREE
#define SAFE_FREE(ptr) {if (NULL != (ptr)) {free(ptr); (ptr) = NULL;}}
#endif

#include "sample_messages.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
// #include <stdio.h>

#include "enclave_invoker.h"

#include <curl/curl.h>


#include <nlohmann/json.hpp>

#include <unistd.h>

using json = nlohmann::json;

void *serve(void *pth_arg) {
	int cfd = (long)pth_arg;

    std::cout << " >>> serve, client fd: " << cfd  <<  std::endl;
    router ru;
    while(true)
    {
        ra_samp_request_header_t *req = RecSampRequestFromClient(cfd);
        if(NULL == req)
        {
            std::cout << " >>> serve, receive ra_samp_request_header_t data is NULL! " << std::endl;
            // closeFd(cfd);
            enclave_invoker::getInstance()->closeSocket(cfd);
            break;
        }

        if(0 != ru.hub(cfd, req))
        {
            std::cout << " >>> serve, 0 != ru.hub " << std::endl;
            enclave_invoker::getInstance()->closeSocket(cfd);
            break;
        }
    }

    return NULL;
}

int server(int port)  
{
    std::cout << " >>> server " << std::endl;

    FILE *OUTPUT = stdout;

    int server_sockfd;//服务器端套接字

    int len;
    struct sockaddr_in my_addr;   //服务器网络地址结构体
    struct sockaddr_in remote_addr; //客户端网络地址结构体
    socklen_t sin_size;
    
    memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零
    my_addr.sin_family = AF_INET; //设置为IP通信
    my_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
    my_addr.sin_port = htons(port); //服务器端口号

    /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
    if((server_sockfd = socket(PF_INET,SOCK_STREAM,0))<0)
    {  
        perror("socket");
        return 1;
    }

        /*将套接字绑定到服务器的网络地址上*/
    if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
    {
        perror("bind");
        return 1;
    }

    std::cout << " >>> server, listening... " << std::endl;
    /*监听连接请求--监听队列长度为5*/
    listen(server_sockfd, 5);

    //使用accept阻塞形式得监听客户端的发来的连接，并返回通信描述符
	long client_sockfd = -1;
	pthread_t id;

    struct sockaddr_in remote_addr = {0};
    socklen_t sin_size = sizeof(remote_addr);
    std::cout << " >>> server, acceptting... " << std::endl;
    client_sockfd = accept(server_sockfd, (struct sockaddr*)&remote_addr, &sin_size);
    if (-1 == client_sockfd) {
        // print_err("accept failed", __LIvNE__, errno);
        continue;
    }
    //建立连接后打印一下客户端的ip和端口号
    printf("\n remot port = %d, remote_addr = %s\n", ntohs(remote_addr.sin_port),inet_ntoa(remote_addr.sin_addr));

    //使用accept返回到描述符，创建子线程进行数据传输
    serve((void*)client_sockfd);
    // int ret = pthread_create(&id,NULL, serve, (void*)client_sockfd);
    // if(-1 == ret) {
    //     // print_err("pthread_create failed", __LINE__, errno); 
    // }

    
    return 0;
}

void *thr_init_sgx_env(void *pth_arg)
{
    int last_taskID = schedule_config::getInstance()->getTaskID();
    int last_totalRound = schedule_config::getInstance()->getTotalRound();
    int last_currentRound = schedule_config::getInstance()->getCurrentRound();

    while(true)
    {
        schedule_config::getInstance()->readConfig(server_config::getInstance()->getSgxID());
        int curr_taskID = schedule_config::getInstance()->getTaskID();
        int curr_totalRound = schedule_config::getInstance()->getTotalRound();
        int curr_currentRound = schedule_config::getInstance()->getCurrentRound();

        if(last_taskID != curr_taskID || last_currentRound != curr_currentRound)
        {
            last_taskID = curr_taskID;
            last_totalRound = curr_totalRound;
            last_currentRound = curr_currentRound;

            std::string sgxID = server_config::getInstance()->getSgxID();
            sgx_status_t status = SGX_SUCCESS;

            std::ifstream in("schedule.json");
            std::ostringstream tmp;
            tmp << in.rdbuf();
            std::string strSchedule = tmp.str();

            int ret = enclave_invoker::getInstance()->initEnclaveEnv_t(enclave_manager::getInstance()->getEnclaveID(),
                            &status,
                            0,
                            (uint8_t *)strSchedule.c_str(), 
                            strSchedule.size(), 
                            (uint8_t *)sgxID.c_str(),
                            sgxID.size());
            if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
            {
                fprintf(stdout, "\nError, initEnclaveEnv failed in [%s]. ret = 0x%0x. status = 0x%0x", __FUNCTION__, ret, status);
            }
        }

        usleep(3000000);
    }

    return 0;

}


bool init(std::string appCfgFileName)
{
    FILE *OUTPUT = stdout;

    if(!server_config::getInstance()->readConfig(appCfgFileName))
    {
        std::cout << " >>> init,read application.json failed. " << std::endl;
        return false;
    }

    enclave_invoker::getInstance()->initSem(server_config::getInstance()->getParallelAggNumber(), server_config::getInstance()->getTcsNum());

    schedule_config::getInstance()->readConfig(server_config::getInstance()->getSgxID());

    if(!enclave_manager::getInstance()->create_enclave())
    {
        std::cout << " >>> init, create_enclave failed. " << std::endl;
        return false;
    }

    std::ifstream in("schedule.json");
    std::ostringstream tmp;
    tmp << in.rdbuf();
    std::string strSchedule = tmp.str();
    // in.close();

    std::string sgxID = server_config::getInstance()->getSgxID();
    sgx_status_t status = SGX_SUCCESS;

    int ret = enclave_invoker::getInstance()->initEnclaveEnv_t(enclave_manager::getInstance()->getEnclaveID(),
                      &status,
                      0,
                      (uint8_t *)strSchedule.c_str(), 
                      strSchedule.size(), 
                      (uint8_t *)sgxID.c_str(),
                      sgxID.size());
    if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status))
    {
        fprintf(OUTPUT, "\nError, initEnclaveEnv failed in [%s]. ret = 0x%0x. status = 0x%0x", __FUNCTION__, ret, status);
        return false;
    }

    pthread_t thrID;
    ret = pthread_create(&thrID,NULL, thr_init_sgx_env, NULL);
    if(-1 == ret) {
        // print_err("pthread_create failed", __LINE__, errno); 
    }        

    return true;
}

int main(int argc, char *argv[])
{
    std::cout << "server start..." << std::endl;

    std::string appCfgFileName = "application.json";
    if (argc > 1)
    {
        appCfgFileName = argv[1];
        std::cout << "application file:" << argv[1] << std::endl;
    }
    std::cout << " >>> main, application config file:" << appCfgFileName << std::endl;

    if(!init(appCfgFileName))
    {
        std::cout << " init failed." << std::endl;
        return -1;
    }

    int port = server_config::getInstance()->getPort();
    std::cout << " server port:" << port << std::endl;
    server(port); 
}



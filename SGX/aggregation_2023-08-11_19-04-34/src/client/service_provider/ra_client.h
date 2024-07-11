
#ifndef _SGX_CLIENT_H
#define _SGX_CLIENT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "network_ra.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 

class sgx_client 
{
public:
    sgx_client();
    ~sgx_client();
    
public:
    int connet(const char sever_ip[16],int port);
    int closeServerFd();

    ra_samp_request_header_t * RecSampRequestFromServer();
    ra_samp_response_header_t * RecSampResponseFromServer();
    int SendSampRequestToServer(const ra_samp_request_header_t *p_req);
    int SendSampResponseToServer(const ra_samp_response_header_t *p_req);

private:
    int server_sockfd;      //服务器端套接字
    // int client_sockfd;      //客户端套接字

    char sendbuf[BUFSIZ];  //数据传送的缓冲区
    char recvbuf[BUFSIZ];  //数据接受的缓冲区
};

#endif

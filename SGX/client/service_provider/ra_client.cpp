#include "ra_client.h"
#include "stdio.h"

#include <errno.h> 

int memcpy_s(
    void *dest,
    size_t numberOfElements,
    const void *src,
    size_t count)
{
    if(numberOfElements<count)
        return -1;
    memcpy(dest, src, count);
    return 0;
}

sgx_client::sgx_client() 
{

}

sgx_client::~sgx_client()
{

}

int sgx_client::connet(const char sever_ip[16],int port) 
{
    struct sockaddr_in remote_addr; //服务器端网络地址结构体
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
    remote_addr.sin_family=AF_INET; //设置为IP通信
    remote_addr.sin_addr.s_addr=inet_addr(sever_ip);//服务器IP地址
    remote_addr.sin_port=htons(port); //服务器端口号

    /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
    if((server_sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket");
        return -1;
    }

    /*将套接字绑定到服务器的网络地址上*/
    if(connect(server_sockfd,(struct sockaddr *)&remote_addr, sizeof(struct sockaddr))<0)
    {
        perror("connect");
        return -1;
    }
    printf("connected to server\n");

    return 0;
}

int sgx_client::closeServerFd()
{
    close(server_sockfd);
}

ra_samp_request_header_t * sgx_client::RecSampRequestFromServer()
{
    const int REQ_HEADER_SIZE = sizeof(ra_samp_request_header_t);
    
	char recvbuf[BUFSIZ];  //数据接受的缓冲区

    ra_samp_request_header_t header;
    int len = 0;
    len = recv(server_sockfd, &header, REQ_HEADER_SIZE,0);
	if(len <= 0) {
        fprintf(stdout, "\nreceive samp request from  Server Error, Exit!\n");
        // printf("\nreceive samp request from  Server Error, Exit!\n");
		return NULL;
	}

    uint32_t dataLen = header.size;
    ra_samp_request_header_t * p_req = (ra_samp_request_header_t *)malloc(REQ_HEADER_SIZE + dataLen);
	memcpy_s(p_req, REQ_HEADER_SIZE, &header, REQ_HEADER_SIZE);

    int RecSize = BUFSIZ;
    // int RecSize = BUFSIZ - 1;
	// int RecSize = 10;
    uint32_t offset = 0;
    while(dataLen > 0) {
        int currRecLen = (dataLen > RecSize) ? RecSize : dataLen; 
        memset(recvbuf, 0, BUFSIZ);
        len = recv(server_sockfd, recvbuf, currRecLen, 0);
        if(len == -1 && errno == EINTR)
        {
            continue;
        }

        memcpy_s(p_req->body + offset, dataLen - offset, recvbuf, len);

        offset += len;
        dataLen -= len;

        printf("offset=%u\n", offset);
    }

    return p_req;

}
 
ra_samp_response_header_t * sgx_client::RecSampResponseFromServer() 
{
    const int RES_HEADER_SIZE = sizeof(ra_samp_response_header_t);
    int len = 0;

	char recvbuf[BUFSIZ];  //数据接受的缓冲区

    ra_samp_response_header_t header;
    len = recv(server_sockfd, &header, RES_HEADER_SIZE,0);
	if(len <= 0) {
		return NULL;
	}

    uint32_t dataLen = header.size;
    ra_samp_response_header_t * p_req = (ra_samp_response_header_t *)malloc(RES_HEADER_SIZE + dataLen);
	memcpy_s(p_req, RES_HEADER_SIZE, &header, RES_HEADER_SIZE);

    int RecSize = BUFSIZ;
    // int RecSize = BUFSIZ - 1;
	// int RecSize = 10;
    uint32_t offset = 0;
    while(dataLen > 0) {
        int currRecLen = (dataLen > RecSize) ? RecSize : dataLen; 
        memset(recvbuf, 0, BUFSIZ);
        len = recv(server_sockfd, recvbuf, currRecLen, 0);
        if(len == -1 && errno == EINTR)
        {
            continue;
        }

        memcpy_s(p_req->body + offset, dataLen - offset, recvbuf, len);

        offset += len;
        dataLen -= len;

        printf("offset=%u\n", offset);
    }

    return p_req;
}

int sgx_client::SendSampRequestToServer(const ra_samp_request_header_t *p_req)
{
    const int RES_HEADER_SIZE = sizeof(ra_samp_request_header_t);

	char sendbuf[BUFSIZ];  //数据接受的缓冲区
	int sendLen = 0;

    int MaxSendSize = BUFSIZ;  //每次最大发送大小
    // int MaxSendSize = BUFSIZ - 1;  //每次最大发送大小
    int ret = 0;
	
	// 发送header
    memset(sendbuf, 0, BUFSIZ);
    memcpy_s(sendbuf, BUFSIZ, p_req, RES_HEADER_SIZE);
    sendLen = send(server_sockfd, sendbuf, RES_HEADER_SIZE, 0);
	if(sendLen != RES_HEADER_SIZE) {
		printf("SendSampRequestToServer,send header fail");
		return -1;
	}

    printf(" \n>>> SendSampRequestToServer,send header ok\n");

	// 发送数据
    uint32_t totalLen = p_req->size;
    uint32_t offset = 0;
    while(totalLen > 0) {
        int currSendLen = (totalLen > MaxSendSize) ? MaxSendSize : totalLen;

        memset(sendbuf, 0, BUFSIZ);
        memcpy_s(sendbuf, BUFSIZ, p_req->body + offset, currSendLen);
        
		sendLen = send(server_sockfd, sendbuf, currSendLen, 0);
        if(sendLen == -1 && errno == EINTR)
        {
            continue;
        }
		if(sendLen != currSendLen) {
			printf("SendSampRequestToServer,send data fail");
			return -1;
		}

        totalLen -= currSendLen;
        offset += currSendLen;

        printf("offset=%u\n", offset);
    }

    printf(" \n>>> SendSampRequestToServer,send p_req->body ok\n");

    return ret;
}

int sgx_client::SendSampResponseToServer(const ra_samp_response_header_t *p_req)
{
    const int RES_HEADER_SIZE = sizeof(ra_samp_response_header_t);

	char sendbuf[BUFSIZ];  //数据接受的缓冲区
	int sendLen = 0;

    int MaxSendSize = BUFSIZ;  //每次最大发送大小
    // int MaxSendSize = BUFSIZ - 1;  //每次最大发送大小
    int ret = 0;
	
	// 发送header
    memset(sendbuf, 0, BUFSIZ);
    memcpy_s(sendbuf, BUFSIZ, p_req, RES_HEADER_SIZE);
    sendLen = send(server_sockfd, sendbuf, RES_HEADER_SIZE, 0);
	if(sendLen != RES_HEADER_SIZE) {
		printf("SendSampResponseToServer,send header fail");
		return -1;
	}

	// 发送数据
    uint32_t totalLen = p_req->size;
    uint32_t offset = 0;
    while(totalLen > 0) {
        int currSendLen = (totalLen > MaxSendSize) ? MaxSendSize : totalLen;

        memset(sendbuf, 0, BUFSIZ);
        memcpy_s(sendbuf, BUFSIZ, p_req->body + offset, currSendLen);
        
		sendLen = send(server_sockfd, sendbuf, currSendLen, 0);
        if(sendLen == -1 && errno == EINTR)
        {
            continue;
        }
		if(sendLen != currSendLen) {
			printf("SendSampResponseToServer,send data fail");
			return -1;
		}

        totalLen -= currSendLen;
        offset += currSendLen;

        printf("offset=%u\n", offset);
    }

    return ret;
}

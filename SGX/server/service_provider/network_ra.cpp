/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "network_ra.h"
#include "service_provider.h"

//add
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <errno.h> 

// Used to send requests to the service provider sample.  It
// simulates network communication between the ISV app and the
// ISV service provider.  This would be modified in a real
// product to use the proper IP communication.
//
// @param server_url String name of the server URL
// @param p_req Pointer to the message to be sent.
// @param p_resp Pointer to a pointer of the response message.

// @return int

int ra_network_send_receive(const char *server_url,
    const ra_samp_request_header_t *p_req,
    ra_samp_response_header_t **p_resp)
{
    int ret = 0;
    ra_samp_response_header_t* p_resp_msg;

    if((NULL == server_url) ||
        (NULL == p_req) ||
        (NULL == p_resp))
    {
        return -1;
    }

    switch(p_req->type)
    {

    case TYPE_RA_MSG0:
        ret = sp_ra_proc_msg0_req((const sample_ra_msg0_t*)((size_t)p_req
            + sizeof(ra_samp_request_header_t)),
            p_req->size,
            &p_resp_msg);
        if (0 != ret)
        {
            fprintf(stderr, "\nError, call sp_ra_proc_msg1_req fail [%s].",
                __FUNCTION__);
        }
        else
        {
            *p_resp = p_resp_msg;
        }
        break;

    case TYPE_RA_MSG1:
        ret = sp_ra_proc_msg1_req((const sample_ra_msg1_t*)((size_t)p_req
            + sizeof(ra_samp_request_header_t)),
            p_req->size,
            &p_resp_msg);
        if(0 != ret)
        {
            fprintf(stderr, "\nError, call sp_ra_proc_msg1_req fail [%s].",
                __FUNCTION__);
        }
        else
        {
            *p_resp = p_resp_msg;
        }
        break;

    case TYPE_RA_MSG3:
        ret =sp_ra_proc_msg3_req((const sample_ra_msg3_t*)((size_t)p_req +
            sizeof(ra_samp_request_header_t)),
            p_req->size,
            &p_resp_msg);
        if(0 != ret)
        {
            fprintf(stderr, "\nError, call sp_ra_proc_msg3_req fail [%s].",
                __FUNCTION__);
        }
        else
        {
            *p_resp = p_resp_msg;
        }
        break;

    default:
        ret = -1;
        fprintf(stderr, "\nError, unknown ra message type. Type = %d [%s].",
            p_req->type, __FUNCTION__);
        break;
    }

    return ret;
}

// Used to free the response messages.  In the sample code, the
// response messages are allocated by the SP code.
//
//
// @param resp Pointer to the response buffer to be freed.

void ra_free_network_response_buffer(ra_samp_response_header_t *resp)
{
    if(resp!=NULL)
    {
        free(resp);
    }
}

// ********** modify ********** //

int ra_network_send_receive2(long clientfd,
                            const ra_samp_request_header_t *req,
                            ra_samp_response_header_t **p_resp)
{
    if((0 == clientfd) ||
        (NULL == req) ||
        (NULL == p_resp))
    {
        fprintf(stderr, "\n>>> ra_network_send_receive2, args error.");
        return -1;
    }

    if(TYPE_RA_MSG0 == req->type ||
       TYPE_RA_MSG1 == req->type ||
       TYPE_RA_MSG3 == req->type) 
    {
        int ret = SendSampRequestToClient(clientfd, req);
        if (0 != ret)
        {
            fprintf(stderr, "\nError, send message0 to client fail [%s].",__FUNCTION__);
            return -1;
        }

        *p_resp = RecSampResponseFromClient(clientfd);
        if (NULL == *p_resp) 
        {
            fprintf(stderr, "\nError, receive message0 response from client fail [%s].",__FUNCTION__);
            return -1;
        }
    } else {
        fprintf(stderr, "\nError, unknown ra message type. Type = %d [%s].", req->type, __FUNCTION__);
        return -1;
    }

    return 0;
}


// void closeFd(int client_sockfd)
// {
//     // printf("\n\n >>> closeFd, socket fd:%ld\n\n", client_sockfd);
//     // close(client_sockfd);
// }

ra_samp_request_header_t * RecSampRequestFromClient(int client_sockfd) 
{
    const int REQ_HEADER_SIZE = sizeof(ra_samp_request_header_t);
    int len = 0;

	char recvbuf[BUFSIZ];  //数据接受的缓冲区

    ra_samp_request_header_t header;
    len = recv(client_sockfd, &header, REQ_HEADER_SIZE,0);
	if(len <= 0) {
        printf("\n\n >>> RecSampRequestFromClient, recv ra_samp_request_header_t failed, client fd:%d\n\n", client_sockfd);
		return NULL;
	}

    uint32_t dataLen = header.size;
    printf("\n\n >>> RecSampRequestFromClient, recv ra_samp_request_header_t ok, client fd:%d, header.size:%d\n\n", client_sockfd, header.size);

    ra_samp_request_header_t * p_req = (ra_samp_request_header_t *)malloc(REQ_HEADER_SIZE + dataLen);
	memcpy_s(p_req, REQ_HEADER_SIZE, &header, REQ_HEADER_SIZE);

    int RecSize = BUFSIZ;
    // int RecSize = BUFSIZ - 1;
    uint32_t offset = 0;
    while(dataLen > 0) { 
        int currRecLen = (dataLen > RecSize) ? RecSize : dataLen; 
        memset(recvbuf, 0, BUFSIZ);
        len = recv(client_sockfd, recvbuf, currRecLen, 0);
        if(len == -1 && errno == EINTR)
        {
            continue;
        }

        if(len == -1)
        {
            free(p_req);
            return NULL;
        }

        memcpy_s(p_req->body + offset, len, recvbuf, len);
        // memcpy_s(p_req->body + offset, dataLen - offset, recvbuf, len);

        offset += len;
        dataLen -= len;

        printf("offset=%u\n", offset);
    }

    printf("\n\n >>> RecSampRequestFromClient, recv data ok, client fd:%d, header.size:%d\n\n", client_sockfd, header.size);

    return p_req;
}

int SendSampRequestToClient(int client_sockfd, const ra_samp_request_header_t  *p_req)
{
    fprintf(stderr, ">>> SendSampRequestToClient...");
    const int REQ_HEADER_SIZE = sizeof(ra_samp_request_header_t);

	char sendbuf[BUFSIZ];  //数据接受的缓冲区
	int sendLen = 0;

    int MaxSendSize = BUFSIZ;  //每次最大发送大小
    // int MaxSendSize = BUFSIZ - 1;  //每次最大发送大小
    int ret = 0;
	
	// 发送header
    memset(sendbuf, 0, BUFSIZ);
    memcpy_s(sendbuf, BUFSIZ, p_req, REQ_HEADER_SIZE);

    fprintf(stderr, ">>> SendSampRequestToClient[send]...");
    sendLen = send(client_sockfd, sendbuf, REQ_HEADER_SIZE, 0);
	if(sendLen != REQ_HEADER_SIZE) {
		fprintf(stderr,"\nSendSampRequestToClient,send header fail");
		return -1;
	}

	// 发送数据
    uint32_t totalLen = p_req->size;
    uint32_t offset = 0;
    while(totalLen > 0) {
        int currSendLen = (totalLen > MaxSendSize) ? MaxSendSize : totalLen;

        memset(sendbuf, 0, BUFSIZ);
        memcpy_s(sendbuf, BUFSIZ, p_req->body + offset, currSendLen);
        
		sendLen = send(client_sockfd, sendbuf, currSendLen, 0);
        if(sendLen == -1 && errno == EINTR)
        {
            continue;
        }
		if(sendLen != currSendLen) {
			fprintf(stderr,"\nsendSampResponseToClient,send data fail");
			return -1;
		}

        totalLen -= currSendLen;
        offset += currSendLen;

        printf("offset=%u\n", offset);
    }

    return ret;
}

ra_samp_response_header_t * RecSampResponseFromClient(int client_sockfd) 
{
    const int REQ_HEADER_SIZE = sizeof(ra_samp_response_header_t);
    int len = 0;

	char recvbuf[BUFSIZ];  //数据接受的缓冲区

    ra_samp_response_header_t header;
    len = recv(client_sockfd, &header, REQ_HEADER_SIZE,0);
	if(len <= 0) {
		return NULL;
	}

    uint32_t dataLen = header.size;
    ra_samp_response_header_t * p_req = (ra_samp_response_header_t *)malloc(REQ_HEADER_SIZE + dataLen);
	memcpy_s(p_req, REQ_HEADER_SIZE, &header, REQ_HEADER_SIZE);

    int RecSize = BUFSIZ;
    // int RecSize = BUFSIZ - 1;
	// int RecSize = 10;
    uint32_t offset = 0;
    while(dataLen > 0) {
        int currRecLen = (dataLen > RecSize) ? RecSize : dataLen; 
        memset(recvbuf, 0, BUFSIZ);
        len = recv(client_sockfd, recvbuf, currRecLen, 0);
        if(len == -1 && errno == EINTR)
        {
            continue;
        }

        if(len == -1)
        {
            free(p_req);
            return NULL;
        }

        memcpy_s(p_req->body + offset, len, recvbuf, len);
        // memcpy_s(p_req->body + offset, dataLen - offset, recvbuf, len);

        offset += len;
        dataLen -= len;

        printf("offset=%u\n", offset);
    }

    return p_req;
}


int SendSampResponseToClient(int client_sockfd, const ra_samp_response_header_t *p_req)
{
    const int REQ_HEADER_SIZE = sizeof(ra_samp_response_header_t);

	char sendbuf[BUFSIZ];  //数据接受的缓冲区
	int sendLen = 0;

    int MaxSendSize = BUFSIZ;  //每次最大发送大小
    // int MaxSendSize = BUFSIZ - 1;  //每次最大发送大小
    int ret = 0;
	
	// 发送header
    memset(sendbuf, 0, BUFSIZ);
    memcpy_s(sendbuf, BUFSIZ, p_req, REQ_HEADER_SIZE);
    sendLen = send(client_sockfd, sendbuf, REQ_HEADER_SIZE, 0);
	if(sendLen != REQ_HEADER_SIZE) {
		printf("sendSampResponseToClient,send header fail");
		return -1;
	}

	// 发送数据
    uint32_t totalLen = p_req->size;
    uint32_t offset = 0;
    while(totalLen > 0) {
        int currSendLen = (totalLen > MaxSendSize) ? MaxSendSize : totalLen;

        memset(sendbuf, 0, BUFSIZ);
        memcpy_s(sendbuf, BUFSIZ, p_req->body + offset, currSendLen);
        
		sendLen = send(client_sockfd, sendbuf, currSendLen, 0);
        if(sendLen == -1 && errno == EINTR)
        {
            continue;
        }
		if(sendLen != currSendLen) {
			printf("sendSampResponseToClient,send data fail");
			return -1;
		}

        totalLen -= currSendLen;
        offset += currSendLen;

        printf("offset=%u\n", offset);
    }

    return ret;
}

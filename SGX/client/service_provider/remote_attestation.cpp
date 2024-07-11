#include "remote_attestation.h"
#include "service_provider.h"
#include "schedule_config.h"
#include "app_config.h"

#include <iostream>
using namespace std;

#ifndef SAFE_FREE
#define SAFE_FREE(ptr)     \
    {                      \
        if (NULL != (ptr)) \
        {                  \
            free(ptr);     \
            (ptr) = NULL;  \
        }                  \
    }
#endif

void PRINT_BYTE_ARRAY(
    FILE *file, void *mem, uint32_t len)
{
    if(!mem || !len)
    {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t *array = (uint8_t *)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for(i = 0; i < len - 1; i++)
    {
        fprintf(file, "0x%x, ", array[i]);
        if(i % 8 == 7) fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

remote_attestation::remote_attestation() 
{
}

remote_attestation::~remote_attestation()
{

}

int remote_attestation::startAttestation(sgx_client &client)
{
    fprintf(stdout, ">>> remote_attestation::startAttestation\n");

    ra_samp_request_header_t *p_req =  (ra_samp_request_header_t*)malloc(sizeof(ra_samp_request_header_t));
    if (NULL == p_req)
    {
        return -1;
    }
    p_req->type = TYPE_DO_RA;
    p_req->size = 0;

    fprintf(stdout, ">>> remote_attestation::startAttestation, p_req->type:%d\n", (int)p_req->type);
    
    return client.SendSampRequestToServer(p_req);
}

int remote_attestation::dealMessage0(sgx_client &client)
{
    fprintf(stdout, ">>> remote_attestation::dealMessage0\n");

    ra_samp_response_header_t *p_resp_msg = NULL;
    ra_samp_request_header_t *p_req = NULL;
        
    //阻塞调用socket
    p_req = client.RecSampRequestFromServer();    // 需要释放 p_req，否则内存泄露
    if(NULL == p_req) 
    {
        fprintf(stdout, "receive samp request from  Server Error, Exit!\n");
        return -1;
    }

    fprintf(stdout, "\nrequest type is %d",p_req->type);
    if(p_req->type != TYPE_RA_MSG0) 
    {
        fprintf(stdout, "\n Receive Message 0 error.");
        SAFE_FREE(p_req);
        return -1;
    }

    int ret = sp_ra_proc_msg0_req((const sample_ra_msg0_t*)((size_t)p_req + sizeof(ra_samp_request_header_t)),
            p_req->size,
            &p_resp_msg);
    
    if (0 != ret)
    {
        fprintf(stderr, "\nError, call sp_ra_proc_msg0_req fail [%s].", __FUNCTION__);
        SAFE_FREE(p_req);
        return -1;
    }

    fprintf(stdout, "\nProcess Message 0 Done");
    
    client.SendSampResponseToServer(p_resp_msg);
    fprintf(stdout, "\nSend Message 0 response Done,send length = %d\n", p_resp_msg->size);
    SAFE_FREE(p_req);
    SAFE_FREE(p_resp_msg);

    return 0;
}

int remote_attestation::dealMessage1(sgx_client &client)
{
    fprintf(stdout, ">>> remote_attestation::dealMessage1\n");

    ra_samp_response_header_t *p_resp_msg = NULL;
    ra_samp_request_header_t *p_req = NULL;
        
    //阻塞调用socket
    p_req = client.RecSampRequestFromServer();    // 需要释放 p_req，否则内存泄露
    if(NULL == p_req) 
    {
        fprintf(stdout, "receive samp request from  Server Error, Exit!\n");
        return -1;
    }

    fprintf(stdout, "\n request type is %d\n",p_req->type);
    if(p_req->type != TYPE_RA_MSG1) 
    {
        fprintf(stdout, "\n Receive Message 1 error.");
        SAFE_FREE(p_req);
        return -1;
    }

    int ret = sp_ra_proc_msg1_req((const sample_ra_msg1_t*)((size_t)p_req + sizeof(ra_samp_request_header_t)),
            p_req->size,
            &p_resp_msg);
    
    if (0 != ret)
    {
        fprintf(stderr, "\nError, call sp_ra_proc_msg1_req fail [%s].", __FUNCTION__);
        SAFE_FREE(p_req);
        return -1;
    }

    fprintf(stdout, "\nProcess Message 1 Done");
    
    client.SendSampResponseToServer(p_resp_msg);
    fprintf(stdout, "\nSend Message 1 response Done,send length = %d\n", p_resp_msg->size);
    SAFE_FREE(p_req);
    SAFE_FREE(p_resp_msg);

    return 0;
}

int remote_attestation::dealMessage3(sgx_client &client)
{
    fprintf(stdout, ">>> remote_attestation::dealMessage3\n");

    ra_samp_response_header_t *p_resp_msg = NULL;
    ra_samp_request_header_t *p_req = NULL;
        
    //阻塞调用socket
    p_req = client.RecSampRequestFromServer();    // 需要释放 p_req，否则内存泄露
    if(NULL == p_req) 
    {
        fprintf(stdout, "receive samp request from  Server Error, Exit!\n");
        return -1;
    }

    fprintf(stdout, "\n request type is %d\n",p_req->type);
    if(p_req->type != TYPE_RA_MSG3) 
    {
        fprintf(stdout, "\n Receive Message 3 error.");
        SAFE_FREE(p_req);
        return -1;
    }

    int ret = sp_ra_proc_msg3_req((const sample_ra_msg3_t*)((size_t)p_req + sizeof(ra_samp_request_header_t)),
            p_req->size,
            &p_resp_msg);
    
    if (0 != ret)
    {
        fprintf(stderr, "\nError, call sp_ra_proc_msg3_req fail [%s].", __FUNCTION__);
        SAFE_FREE(p_req);
        return -1;
    }

    fprintf(stdout, "\nProcess Message 3 Done");
    
    client.SendSampResponseToServer(p_resp_msg);
    fprintf(stdout, "\nSend Message 3 response Done,send length = %d\n", p_resp_msg->size);
    SAFE_FREE(p_req);
    SAFE_FREE(p_resp_msg);

    return 0;
}

int remote_attestation::attest(sgx_client &client)
{
    FILE *OUTPUT = stdout;

    int ret = 0;

    ret = startAttestation(client);
    if(0 != ret) {
        return ret;
    }

    ret = dealMessage0(client);
    if(0 != ret) {
        return ret;
    }

    ret = dealMessage1(client);
    if(0 != ret) {
        return ret;
    }

    ret = dealMessage3(client);
    if(0 != ret) {
        return ret;
    }

    return ret;
}



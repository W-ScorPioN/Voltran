#ifndef _REMOTE_ATTESTATION_H
#define _REMOTE_ATTESTATION_H

#include <limits.h>

#include <map>
#include <string>
#include <mutex>

#include "sgx_eid.h"
#include "sgx_ukey_exchange.h"

#include "network_ra.h"

class remote_attestation
{
private:

public:
    remote_attestation(long cfd);
    ~remote_attestation();

public:
    int doAttestation(sgx_enclave_id_t enclave_id);
    sgx_ra_context_t getRaContext();
    void release();

private:
    static std::map<std::string, sgx_ra_context_t> clientID2Context;
    // static std::mutex m_Mutex_;

private:
    long client_sockfd;

    ra_samp_request_header_t *p_msg0_full = NULL;
    ra_samp_response_header_t *p_msg0_resp_full = NULL;
    ra_samp_request_header_t *p_msg1_full = NULL;
    ra_samp_response_header_t *p_msg2_full = NULL;
    ra_samp_request_header_t *p_msg3_full = NULL;
    sgx_att_key_id_t selected_key_id = {0};
    sgx_ra_msg3_t *p_msg3 = NULL;
    ra_samp_response_header_t *p_att_result_msg_full = NULL;
    sgx_enclave_id_t enclave_id = 0;
    int enclave_lost_retry_time = 1;
    int busy_retry_time = 4;
    sgx_ra_context_t context = INT_MAX;
    sgx_status_t status = SGX_SUCCESS;
};


#endif
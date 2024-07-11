#ifndef _SERVER_APP_AGGREGATION_H
#define _SERVER_APP_AGGREGATION_H

#include <limits.h>
#include <string>
#include <set>

#include "sgx_eid.h"
#include "sgx_ukey_exchange.h"

#include "network_ra.h"
#include "request_data_header.h"

class aggregation
{
public:
    aggregation(long cfd, sgx_enclave_id_t enclaveID, sgx_ra_context_t context);
    ~aggregation();

private:
    void ByteToHexStr(const unsigned char* source, char* dest, uint32_t sourceLen);

    int send_wt1_to_chain_loop();
    int send_wt1_to_chain_by_pack(int index, std::set<int> keys);

    void sendWt1ToChain(std::string jsonData);
public:
    int aggregate_client(ra_samp_request_header_t *req, uint8_t* msk);

    int send_wt1_to_chain();    
private:
    long client_sockfd;
    sgx_enclave_id_t enclave_id = 0; 
    sgx_ra_context_t context = INT_MAX;
    sgx_status_t status = SGX_SUCCESS;
};


#endif
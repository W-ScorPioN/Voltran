#ifndef _APP_ROUTER_H
#define _APP_ROUTER_H

#include <limits.h>
#include <mutex>          // std::mutex

#include "network_ra.h"
#include "agg_timer.h"
#include "sgx_ukey_exchange.h"


class router
{
public:
    router();
    ~router();

public:
    int hub(int clientFd, ra_samp_request_header_t *req);
    uint8_t* getMsk(long cfd, sgx_enclave_id_t enclaveID, sgx_ra_context_t context, ra_samp_request_header_t *req);
    uint8_t* msk;

private:
    sgx_ra_context_t context = INT_MAX;

};



#endif
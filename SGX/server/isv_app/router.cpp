#include "router.h"
#include "remote_attestation.h"
#include "enclave_manager.h"
#include "aggregation.h"
#include "util.h"

#include <iostream>

router::router()
{
    msk = nullptr;
}

router::~router()
{
}

uint8_t* getMsk(long cfd, sgx_enclave_id_t enclaveID, sgx_ra_context_t context, ra_samp_request_header_t *req)
{
    FILE *OUTPUT = stdout;
    if(NULL == req) 
    {
        return nullptr;
    }
    sp_aes_gcm_data_t *p_secret = (sp_aes_gcm_data_t *)req->body;

    agg_timer aggTimer;
    aggTimer.setTip("time_count");
    aggTimer.start();

    uint32_t isFinish;

    uint8_t* msk = enclave_invoker::getInstance()->enclave_getMsk(
        enclave_id, 
        &status,
        context,
        p_secret->payload,
        p_secret->payload_size,
        p_secret->payload_tag,
        &isFinish);
    
    aggTimer.end();
    aggTimer.printTimeStamp();

    return msk;
}
int router::hub(int clientFd, ra_samp_request_header_t *req)
{
    std::cout << " >>> router::hub, " << "clientFd:" << clientFd << ", req->type:" << (int)req->type << std::endl;
    if(TYPE_DO_RA == req->type)
    {
        std::cout << " >>> router::hub, TYPE_DO_RA" << std::endl;

        remote_attestation ra(clientFd);
        ra.doAttestation(enclave_manager::getInstance()->getEnclaveID());
        context = ra.getRaContext();
        ra.release();

        SAFE_FREE(req);
        
        return 0;
    }
    // 在这再定义一个类型，SEND_KEY,表示传过来后续加密要用的密钥
    if(TYPE_TTP_SEND_KEY == req->type)
    {
        // TODO: 等client的数据来了之后，再对msk做解密，并一起进行操作
        // 传过来密钥如何操作
        msk = getMsk(clientFd, enclave_manager::getInstance()->getEnclaveID(), context, req);

    }
    if(TYPE_CLIENT_DATA == req->type)
    {
        std::cout << " >>> router::hub, TYPE_CLIENT_AGGR_DATA, sgx context=" << context << std::endl;
        // TODO: 把msk传过去，然后转成密钥的类型，然后对client的数据做解密之后，再用msk加密
        aggregation agg(clientFd, enclave_manager::getInstance()->getEnclaveID(), context, msk);
        agg.aggregate_client(req);

        std::cout << " >>> router::hub, TYPE_CLIENT_AGGR_DATA, free req." << std::endl;

        SAFE_FREE(req);
        return 0;
    }

    std::cout << " >>> router::hub, error req->type:" << (int)req->type << std::endl;

    return -1;
}



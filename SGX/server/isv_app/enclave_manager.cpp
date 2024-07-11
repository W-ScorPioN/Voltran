#include "enclave_manager.h"

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

#include <iostream>

enclave_manager *enclave_manager::instance = NULL;
std::mutex enclave_manager::m_Mutex;
std::string enclave_manager::ENCLAVE_PATH = "isv_enclave.signed.so";

enclave_manager::enclave_manager(/* args */)
{
    enclave_id = 0;
}

enclave_manager::~enclave_manager()
{
}

enclave_manager*  enclave_manager::getInstance()
{
    if (instance == NULL) 
    {
        std::unique_lock<std::mutex> lock(m_Mutex); // 加锁
        if (instance == NULL)
        {
            instance = new enclave_manager();
        }
    }

    return instance;
}

bool enclave_manager::create_enclave()
{
    FILE *OUTPUT = stdout;

    std::cout << " >>> enclave_manager::create_enclave... " << std::endl;

    // fprintf(OUTPUT, "\n >>> create_enclave");

    int ret = SGX_SUCCESS;
    int launch_token_update = 0;
    sgx_launch_token_t launch_token = {0};
    memset(&launch_token, 0, sizeof(sgx_launch_token_t));
    ret = sgx_create_enclave(ENCLAVE_PATH.c_str(),
                            SGX_DEBUG_FLAG,
                            &launch_token,
                            &launch_token_update,
                            &enclave_id, NULL);
    if (SGX_SUCCESS != ret)
    {
        ret = -1;
        fprintf(OUTPUT, "\nError, call sgx_create_enclave fail [%s].\n", __FUNCTION__);

        return false;
    }
    fprintf(OUTPUT, "\nCall sgx_create_enclave success,enclave_id:%d\n", enclave_id);

    return true;
}

sgx_enclave_id_t enclave_manager::getEnclaveID()
{
    return enclave_id;
}




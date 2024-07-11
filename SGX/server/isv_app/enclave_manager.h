#ifndef _ENCLAVE_MANAGER_H
#define _ENCLAVE_MANAGER_H

#include <mutex>
#include <string>

#include "sgx_eid.h"

class enclave_manager
{
private:
	static enclave_manager* instance;
    static std::mutex m_Mutex;

    static std::string ENCLAVE_PATH;
public:
	static enclave_manager* getInstance();

private:
    enclave_manager();
    ~enclave_manager();
public:
    bool create_enclave();
    sgx_enclave_id_t getEnclaveID();
private:
    sgx_enclave_id_t enclave_id; 
};




#endif

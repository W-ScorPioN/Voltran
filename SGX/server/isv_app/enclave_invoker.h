#ifndef _ENCLAVE_INVOKER_H
#define _ENCLAVE_INVOKER_H

#include "sgx_eid.h"
#include "sgx_ukey_exchange.h"

#include "semaphore.h"

#include <mutex>          // std::mutex

class enclave_invoker
{
private:
    /* data */
private:
    enclave_invoker();
    ~enclave_invoker();

    static enclave_invoker instance;
public:
    static enclave_invoker* getInstance();
    void initSem(int count, int tcsNum);
public: 
    sgx_status_t enclave_init_ra_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        int b_pse,
        sgx_ra_context_t *p_context
    );

    sgx_status_t enclave_ra_close_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context);

    sgx_status_t sgx_ra_get_msg1_ex_t(
        const sgx_att_key_id_t *p_att_key_id,
        sgx_ra_context_t context,
        sgx_enclave_id_t eid,
        sgx_ecall_get_ga_trusted_t p_get_ga,
        sgx_ra_msg1_t *p_msg1);
    sgx_status_t sgx_ra_proc_msg2_ex_t(
        const sgx_att_key_id_t *p_att_key_id,
        sgx_ra_context_t context,
        sgx_enclave_id_t eid,
        sgx_ecall_proc_msg2_trusted_t p_proc_msg2,
        sgx_ecall_get_msg3_trusted_t p_get_msg3,
        const sgx_ra_msg2_t *p_msg2,
        uint32_t msg2_size,
        sgx_ra_msg3_t **pp_msg3,
        uint32_t *p_msg3_size);

    sgx_status_t initEnclaveEnv_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context,
        uint8_t* p_scheduleCfg,
        uint32_t cfg_size,
        uint8_t* p_sgxID,
        uint32_t sgxID_size);

    sgx_status_t aggregating_client_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context, 
        uint8_t* p_model, 
        uint32_t model_size, 
        uint8_t* p_gcm_mac,  
        uint32_t *p_finish,
        uint8_t* msk);
    
    sgx_status_t get_mult_sgx_agg_status_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context,
        uint32_t *p_finish);
    
    sgx_status_t get_wt1_and_sign_size_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context,
        uint32_t *p_taskID,
        uint32_t *p_round,
        uint32_t *p_wt1_size,
        uint32_t *p_wt1_mac_size,
        uint32_t *p_signWt1_size);
    
    sgx_status_t get_wt1_and_sign_size_by_key_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context,
        uint8_t* p_keys,
        uint32_t key_size,
        uint32_t *p_taskID,
        uint32_t *p_round,
        uint32_t *p_wt1_size,
        uint32_t *p_wt1_mac_size,
        uint32_t *p_signWt1_size);
    
    sgx_status_t get_wt1_and_sign_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context,
        uint8_t* p_wt1,
        uint32_t out_size_wt1,
        uint8_t* p_wt1_mac,
        uint32_t out_size_wt1_mac,
        uint8_t* p_sign,
        uint32_t out_size_sign);
    sgx_status_t get_wt1_and_sign_by_key_t(
        sgx_enclave_id_t enclave_id,
        sgx_status_t *status,
        sgx_ra_context_t context,
        uint8_t* p_keys,
        uint32_t key_size,
        uint8_t* p_wt1,
        uint32_t out_size_wt1,
        uint8_t* p_wt1_mac,
        uint32_t out_size_wt1_mac,
        uint8_t* p_sign,
        uint32_t out_size_sign);
    
    void closeSocket(int fd);
private:
    std::mutex m_mutex;

    // semaphore m_sem;
    semaphore m_tcs_sem;
};



#endif
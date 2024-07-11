#include "enclave_invoker.h"

#include "isv_enclave_u.h"
#include "sgx_error.h"

#include <unistd.h>

enclave_invoker enclave_invoker::instance;

enclave_invoker::enclave_invoker()
{
}

enclave_invoker::~enclave_invoker()
{
}

enclave_invoker * enclave_invoker::getInstance()
{
    return &instance;
}

void enclave_invoker::initSem(int aggCount, int tcsNum)
{
    // m_sem.init(aggCount);
    m_tcs_sem.init(tcsNum);
}

sgx_status_t enclave_invoker::enclave_init_ra_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    int b_pse,
    sgx_ra_context_t *p_context)
{
    std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = enclave_init_ra(enclave_id, status, b_pse, p_context);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::enclave_ra_close_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context)
{
    m_tcs_sem.wait();
    sgx_status_t ret = enclave_ra_close(enclave_id, status, context);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::sgx_ra_get_msg1_ex_t(
    const sgx_att_key_id_t *p_att_key_id,
    sgx_ra_context_t context,
    sgx_enclave_id_t eid,
    sgx_ecall_get_ga_trusted_t p_get_ga,
    sgx_ra_msg1_t *p_msg1)
{
    m_tcs_sem.wait();
    sgx_status_t ret = sgx_ra_get_msg1_ex(p_att_key_id, context, eid, p_get_ga, p_msg1);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::sgx_ra_proc_msg2_ex_t(
    const sgx_att_key_id_t *p_att_key_id,
    sgx_ra_context_t context,
    sgx_enclave_id_t eid,
    sgx_ecall_proc_msg2_trusted_t p_proc_msg2,
    sgx_ecall_get_msg3_trusted_t p_get_msg3,
    const sgx_ra_msg2_t *p_msg2,
    uint32_t msg2_size,
    sgx_ra_msg3_t **pp_msg3,
    uint32_t *p_msg3_size)
{
    m_tcs_sem.wait();
    sgx_status_t ret = sgx_ra_proc_msg2_ex(p_att_key_id, context, eid, p_proc_msg2, p_get_msg3,p_msg2,msg2_size,pp_msg3,p_msg3_size);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::initEnclaveEnv_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context,
    uint8_t* p_scheduleCfg,
    uint32_t cfg_size,
    uint8_t* p_sgxID,
    uint32_t sgxID_size)
{
    // std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = initEnclaveEnv(enclave_id, status, context, p_scheduleCfg, cfg_size, p_sgxID, sgxID_size);
    m_tcs_sem.signal();

    return ret;
}


sgx_status_t enclave_invoker::enclave_getMsk(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context, 
    uint8_t* p_data, 
    uint32_t data_size, 
    uint8_t* p_gcm_mac,  
    uint32_t *p_finish)
{
    //  std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = aggregating_client(enclave_id, status, context, p_data, data_size, p_gcm_mac, p_finish);
    m_tcs_sem.signal();

    return ret;
}


sgx_status_t enclave_invoker::aggregating_client_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context, 
    uint8_t* p_data, 
    uint32_t data_size, 
    uint8_t* p_gcm_mac,  
    uint32_t *p_finish,
    uint8_t* msk)
{
    //  std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = aggregating_client(enclave_id, status, context, p_data, data_size, p_gcm_mac, p_finish);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::get_mult_sgx_agg_status_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context,
    uint32_t *p_finish)
{
    // std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = get_mult_sgx_agg_status(enclave_id, status, context, p_finish);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::get_wt1_and_sign_size_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context,
    uint32_t *p_taskID,
    uint32_t *p_round,
    uint32_t *p_wt1_size,
    uint32_t *p_wt1_mac_size,
    uint32_t *p_signWt1_size)
{
    // std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = get_wt1_and_sign_size(enclave_id, status, context, p_taskID, p_round, p_wt1_size, p_wt1_mac_size, p_signWt1_size);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::get_wt1_and_sign_size_by_key_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context,
    uint8_t *p_keys,
    uint32_t key_size,
    uint32_t *p_taskID,
    uint32_t *p_round,
    uint32_t *p_wt1_size,
    uint32_t *p_wt1_mac_size,
    uint32_t *p_signWt1_size)
{
    // std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = get_wt1_and_sign_size_by_key(enclave_id, status, context, p_keys, key_size, p_taskID, p_round, p_wt1_size, p_wt1_mac_size, p_signWt1_size);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::get_wt1_and_sign_t(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context,
    uint8_t* p_wt1,
    uint32_t out_size_wt1,
    uint8_t* p_wt1_mac,
    uint32_t out_size_wt1_mac,
    uint8_t* p_sign,
    uint32_t out_size_sign)
{
    // std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = get_wt1_and_sign(enclave_id, status, context, p_wt1, out_size_wt1, p_wt1_mac, out_size_wt1_mac, p_sign, out_size_sign);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::get_wt1_and_sign_by_key_t(
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
    uint32_t out_size_sign)
{
    // std::unique_lock<std::mutex> lck (m_mutex);
    m_tcs_sem.wait();
    sgx_status_t ret = get_wt1_and_sign_by_key(enclave_id, status, context, p_keys, key_size, p_wt1, out_size_wt1, p_wt1_mac, out_size_wt1_mac, p_sign, out_size_sign);
    m_tcs_sem.signal();

    return ret;
}

sgx_status_t enclave_invoker::get_data_and_encrypt(
    sgx_enclave_id_t enclave_id,
    sgx_status_t *status,
    sgx_ra_context_t context,
)
{
    return ret;
}

void enclave_invoker::closeSocket(int fd)
{
    // printf("\n\n >>> enclave_invoker::closeSocket, socket fd:%ld\n\n", fd);

    m_tcs_sem.wait();
    close(fd);
    m_tcs_sem.signal();
    
    return;
}


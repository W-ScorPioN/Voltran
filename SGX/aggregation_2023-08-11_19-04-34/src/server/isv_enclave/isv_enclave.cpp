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


#include <assert.h>
#include "isv_enclave_t.h"
#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"
#include "sgx_thread.h"
#include <iostream>

#include "string.h"

#include "aggregation_data.h"
#include "request_data_header.h"
#include "sgx_invoke_status.h"


#include <map>
#include <stdio.h>

static const sgx_ec256_private_t g_priv_key = {
    {
        0xEB, 0xA7, 0xA9, 0x4E, 0xA8, 0x8B, 0xE6, 0x93, 
        0x86, 0x7C, 0x29, 0xED, 0x58, 0xEB, 0x9A, 0x6C, 
        0x94, 0x5D, 0x95, 0x90, 0x52, 0x24, 0xE4, 0x42, 
        0x47, 0x86, 0x17, 0xF6, 0x63, 0x2C, 0x35, 0xBE 
    }
};

static const sgx_aes_gcm_128bit_key_t g_aes_key = {
    0xBE, 0xE1, 0x34, 0x79, 0x41, 0x82, 0x7D, 0xAB,
    0xD8, 0xE8, 0xB1, 0x12, 0x6C, 0xDC, 0x6F, 0xD1
};

// This is the public EC key of the SP. The corresponding private EC key is
// used by the SP to sign data used in the remote attestation SIGMA protocol
// to sign channel binding data in MSG2. A successful verification of the
// signature confirms the identity of the SP to the ISV app in remote
// attestation secure channel binding. The public EC key should be hardcoded in
// the enclave or delivered in a trustworthy manner. The use of a spoofed public
// EC key in the remote attestation with secure channel binding session may lead
// to a security compromise. Every different SP the enclave communicates to
// must have a unique SP public key. Delivery of the SP public key is
// determined by the ISV. The TKE SIGMA protocol expects an Elliptical Curve key
// based on NIST P-256
static const sgx_ec256_public_t g_sp_pub_key = {
    {
        0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
    },
    {
        0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
    }

};

// Used to store the secret passed by the SP in the sample code. The
// size is forced to be 8 bytes. Expected value is
// 0x01,0x02,0x03,0x04,0x0x5,0x0x6,0x0x7
uint8_t g_secret[8] = {0};


static sgx_thread_mutex_t global_mutex = SGX_THREAD_MUTEX_INITIALIZER;

static aggregation_data g_aggData;

#ifdef SUPPLIED_KEY_DERIVATION

#pragma message ("Supplied key derivation function is used.")

typedef struct _hash_buffer_t
{
    uint8_t counter[4];
    sgx_ec256_dh_shared_t shared_secret;
    uint8_t algorithm_id[4];
} hash_buffer_t;



const char ID_U[] = "SGXRAENCLAVE";
const char ID_V[] = "SGXRASERVER";

// Derive two keys from shared key and key id.
bool derive_key(
    const sgx_ec256_dh_shared_t *p_shared_key,
    uint8_t key_id,
    sgx_ec_key_128bit_t *first_derived_key,
    sgx_ec_key_128bit_t *second_derived_key)
{
    sgx_status_t sgx_ret = SGX_SUCCESS;
    hash_buffer_t hash_buffer;
    sgx_sha_state_handle_t sha_context;
    sgx_sha256_hash_t key_material;

    memset(&hash_buffer, 0, sizeof(hash_buffer_t));
    /* counter in big endian  */
    hash_buffer.counter[3] = key_id;

    /*convert from little endian to big endian */
    for (size_t i = 0; i < sizeof(sgx_ec256_dh_shared_t); i++)
    {
        hash_buffer.shared_secret.s[i] = p_shared_key->s[sizeof(p_shared_key->s)-1 - i];
    }

    sgx_ret = sgx_sha256_init(&sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t*)&hash_buffer, sizeof(hash_buffer_t), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t*)&ID_U, sizeof(ID_U), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t*)&ID_V, sizeof(ID_V), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_get_hash(sha_context, &key_material);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_close(sha_context);

    assert(sizeof(sgx_ec_key_128bit_t)* 2 == sizeof(sgx_sha256_hash_t));
    memcpy(first_derived_key, &key_material, sizeof(sgx_ec_key_128bit_t));
    memcpy(second_derived_key, (uint8_t*)&key_material + sizeof(sgx_ec_key_128bit_t), sizeof(sgx_ec_key_128bit_t));

    // memset here can be optimized away by compiler, so please use memset_s on
    // windows for production code and similar functions on other OSes.
    memset(&key_material, 0, sizeof(sgx_sha256_hash_t));

    return true;
}

//isv defined key derivation function id
#define ISV_KDF_ID 2

typedef enum _derive_key_type_t
{
    DERIVE_KEY_SMK_SK = 0,
    DERIVE_KEY_MK_VK,
} derive_key_type_t;

sgx_status_t key_derivation(const sgx_ec256_dh_shared_t* shared_key,
    uint16_t kdf_id,
    sgx_ec_key_128bit_t* smk_key,
    sgx_ec_key_128bit_t* sk_key,
    sgx_ec_key_128bit_t* mk_key,
    sgx_ec_key_128bit_t* vk_key)
{
    bool derive_ret = false;

    if (NULL == shared_key)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    if (ISV_KDF_ID != kdf_id)
    {
        //fprintf(stderr, "\nError, key derivation id mismatch in [%s].", __FUNCTION__);
        return SGX_ERROR_KDF_MISMATCH;
    }

    derive_ret = derive_key(shared_key, DERIVE_KEY_SMK_SK,
        smk_key, sk_key);
    if (derive_ret != true)
    {
        //fprintf(stderr, "\nError, derive key fail in [%s].", __FUNCTION__);
        return SGX_ERROR_UNEXPECTED;
    }

    derive_ret = derive_key(shared_key, DERIVE_KEY_MK_VK,
        mk_key, vk_key);
    if (derive_ret != true)
    {
        //fprintf(stderr, "\nError, derive key fail in [%s].", __FUNCTION__);
        return SGX_ERROR_UNEXPECTED;
    }
    return SGX_SUCCESS;
}
#else
#pragma message ("Default key derivation function is used.")
#endif

// This ecall is a wrapper of sgx_ra_init to create the trusted
// KE exchange key context needed for the remote attestation
// SIGMA API's. Input pointers aren't checked since the trusted stubs
// copy them into EPC memory.
//
// @param b_pse Indicates whether the ISV app is using the
//              platform services.
// @param p_context Pointer to the location where the returned
//                  key context is to be copied.
//
// @return Any error returned from the trusted key exchange API
//         for creating a key context.

sgx_status_t enclave_init_ra(
    int b_pse,
    sgx_ra_context_t *p_context)
{
    // sgx_thread_mutex_lock(&global_mutex);

    // isv enclave call to trusted key exchange library.
    sgx_status_t ret;
#ifdef SUPPLIED_KEY_DERIVATION
    ret = sgx_ra_init_ex(&g_sp_pub_key, b_pse, key_derivation, p_context);
#else
    ret = sgx_ra_init(&g_sp_pub_key, b_pse, p_context);
#endif

    // sgx_thread_mutex_unlock(&global_mutex);

    return ret;
}


// Closes the tKE key context used during the SIGMA key
// exchange.
//
// @param context The trusted KE library key context.
//
// @return Return value from the key context close API

sgx_status_t SGXAPI enclave_ra_close(
    sgx_ra_context_t context)
{
    sgx_status_t ret;
    ret = sgx_ra_close(context);
    return ret;
}


// Verify the mac sent in att_result_msg from the SP using the
// MK key. Input pointers aren't checked since the trusted stubs
// copy them into EPC memory.
//
//
// @param context The trusted KE library key context.
// @param p_message Pointer to the message used to produce MAC
// @param message_size Size in bytes of the message.
// @param p_mac Pointer to the MAC to compare to.
// @param mac_size Size in bytes of the MAC
//
// @return SGX_ERROR_INVALID_PARAMETER - MAC size is incorrect.
// @return Any error produced by tKE  API to get SK key.
// @return Any error produced by the AESCMAC function.
// @return SGX_ERROR_MAC_MISMATCH - MAC compare fails.

sgx_status_t verify_att_result_mac(sgx_ra_context_t context,
                                   uint8_t* p_message,
                                   size_t message_size,
                                   uint8_t* p_mac,
                                   size_t mac_size)
{
    sgx_status_t ret;
    sgx_ec_key_128bit_t mk_key;

    if(mac_size != sizeof(sgx_mac_t))
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }
    if(message_size > UINT32_MAX)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }

    do {
        uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};

        ret = sgx_ra_get_keys(context, SGX_RA_KEY_MK, &mk_key);
        if(SGX_SUCCESS != ret)
        {
            break;
        }
        ret = sgx_rijndael128_cmac_msg(&mk_key,
                                       p_message,
                                       (uint32_t)message_size,
                                       &mac);
        if(SGX_SUCCESS != ret)
        {
            break;
        }
        if(0 == consttime_memequal(p_mac, mac, sizeof(mac)))
        {
            ret = SGX_ERROR_MAC_MISMATCH;
            break;
        }

    }
    while(0);

    return ret;
}

sgx_status_t initEnclaveEnv(sgx_ra_context_t context, 
    uint8_t* p_scheduleCfg,
    uint32_t cfg_size,
    uint8_t* p_sgxID,
    uint32_t sgxID_size)
{
    sgx_thread_mutex_lock(&global_mutex);

    g_aggData.resetEnv();
    g_aggData.parseSchedule((char *)p_scheduleCfg, (char *)p_sgxID);

    sgx_thread_mutex_unlock(&global_mutex);

    return SGX_SUCCESS;
}



uint8_t* decryptMsk(
    sgx_ra_context_t context, 
    uint8_t* p_data,
    uint32_t data_size,
    uint8_t* p_gcm_mac,
    uint32_t *p_finish)
{
    sgx_status_t ret = SGX_SUCCESS;
    sgx_ra_key_128_t sk_key;

    *p_finish = -1;

    uint8_t *p_out = (uint8_t *)malloc(data_size); // 存放解密后的client数据
    if(!p_out)
    {
        std::cout << "SGX_ERROR_OUT_OF_MEMORY" << std::endl;
        return nullptr;
    }


    ret = sgx_ra_get_keys(context, SGX_RA_KEY_SK, &sk_key);
    if(SGX_SUCCESS != ret)
    {
       std::cout << "SGX_GET_KEY_FAILED" << std::endl;
       return nullptr;
    }

    uint8_t aes_gcm_iv[12] = {0};
    ret = sgx_rijndael128GCM_decrypt(&sk_key,
                                        p_data,
                                        data_size,
                                        p_out,
                                        &aes_gcm_iv[0],
                                        12,
                                        NULL,
                                        0,
                                        (const sgx_aes_gcm_128bit_tag_t *)(p_gcm_mac)); // 解密
    if(SGX_SUCCESS != ret)
    {
        std::cout << "SGX_DECRYPT_KEY_FAILED" << std::endl;
        return nullptr;
    }

    
    return p_out;
}


sgx_status_t aggregating_client(
    sgx_ra_context_t context, 
    uint8_t* p_data,
    uint32_t data_size,
    uint8_t* p_gcm_mac,
    uint32_t *p_finish,
    uint8_t* msk)
{
    sgx_status_t ret = SGX_SUCCESS;
    sgx_ra_key_128_t sk_key;

    *p_finish = -1;

    uint8_t *p_out = (uint8_t *)malloc(data_size); // 存放解密后的client数据
    if(!p_out)
    {
        return SGX_ERROR_OUT_OF_MEMORY;
    }

    // sgx_thread_mutex_lock(&global_mutex);

    // do {
    ret = sgx_ra_get_keys(context, SGX_RA_KEY_SK, &sk_key);
    if(SGX_SUCCESS != ret)
    {
        break;
    }

    uint8_t aes_gcm_iv[12] = {0};
    ret = sgx_rijndael128GCM_decrypt(&sk_key,
                                        p_data,
                                        data_size,
                                        p_out,
                                        &aes_gcm_iv[0],
                                        12,
                                        NULL,
                                        0,
                                        (const sgx_aes_gcm_128bit_tag_t *)(p_gcm_mac)); // 解密
    if(SGX_SUCCESS != ret)
    {
        break;
    }


    uint8_t aes_iv[12] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00
    };

    uint32_t p_wt1_mac_size = sizeof(sgx_aes_gcm_128bit_tag_t);
    uint8_t* p_wt1_mac = (uint8_t *)malloc(wt1_mac_size + 1);
    uint8_t* p_wt1 = (uint8_t *)malloc(data_size + 1);
    ret = sgx_rijndael128GCM_encrypt((const sgx_ra_key_128_t *)(msk),
                                    p_out,
                                    data_size,
                                    (uint8_t *)p_wt1,
                                    &aes_iv[0],
                                    12,
                                    NULL,
                                    0,
                                    (sgx_aes_gcm_128bit_tag_t *)p_wt1_mac);
    
    std::cout << "| ciphertext | " ;
    for(int i = 0; i < data_size; i++)
    {
        std::cout << p_wt1[i] << " ";
    }
    std::cout << " |" << std::endl;

    std::cout << "| tag | " ;
    for(int i = 0; i < p_wt1_mac_size; i++)
    {
        std::cout << p_wt1_mac[i] << " ";
    }
    std::cout << " |" << std::endl;
    // ret = g_aggData.aggregate_client((request_data_header_t *)p_out);


    // // 加密
    // if(g_aggData.isAggregatingClientFinish())
    // {
    //     *p_finish = SGX_AGG_FINISH;
    // } else
    // {
    //     *p_finish = SGX_AGG_NOT_FINISH;
    // }

    // if(SGX_SUCCESS != ret)
    // {
    //     break;
    // }

    // // } while (0);

    // // sgx_thread_mutex_unlock(&global_mutex);

    free(p_out);

    return ret;
}

sgx_status_t get_mult_sgx_agg_status(
    sgx_ra_context_t context, 
    uint32_t *p_finish)
{
    sgx_thread_mutex_lock(&global_mutex);

    if(g_aggData.isAggregatingSgxFinish())
    {
        *p_finish = SGX_MULT_AGG_FINISH;
    } else {
         *p_finish = SGX_MULT_AGG_NOT_FINISH;
    }

    sgx_thread_mutex_unlock(&global_mutex);

    return SGX_SUCCESS;
}

sgx_status_t get_wt1_and_sign_size(
    sgx_ra_context_t context,
    uint32_t *p_taskID,
    uint32_t *p_round,
    uint32_t *p_wt1_size,
    uint32_t *p_wt1_mac_size,
    uint32_t *p_signWt1_size)
{
    sgx_thread_mutex_lock(&global_mutex);
    sgx_status_t ret = g_aggData.get_wt1_and_sign_size(p_taskID, p_round, p_wt1_size, p_wt1_mac_size, p_signWt1_size);
    sgx_thread_mutex_unlock(&global_mutex);

    return ret;
}

sgx_status_t get_wt1_and_sign_size_by_key(
    sgx_ra_context_t context,
    uint8_t* p_keys,
    uint32_t key_size,
    uint32_t *p_taskID,
    uint32_t *p_round,
    uint32_t *p_wt1_size,
    uint32_t *p_wt1_mac_size,
    uint32_t *p_signWt1_size)
{
    sgx_thread_mutex_lock(&global_mutex);
    sgx_status_t ret = g_aggData.get_wt1_and_sign_size_by_key(p_keys, key_size, p_taskID, p_round, p_wt1_size, p_wt1_mac_size, p_signWt1_size);
    sgx_thread_mutex_unlock(&global_mutex);

    return ret;
}

sgx_status_t get_wt1_and_sign(
    sgx_ra_context_t context,
    uint8_t* p_wt1,
    uint32_t out_size_wt1,
    uint8_t* p_wt1_mac,
    uint32_t out_size_wt1_mac,
    uint8_t* p_sign,
    uint32_t out_size_sign)
{
    sgx_thread_mutex_lock(&global_mutex);
    sgx_status_t ret = g_aggData.get_wt1_and_sign(&g_aes_key, &g_priv_key, p_wt1, out_size_wt1, p_wt1_mac, out_size_wt1_mac, p_sign, out_size_sign);
    sgx_thread_mutex_unlock(&global_mutex);

    return ret;
}

sgx_status_t get_wt1_and_sign_by_key(
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
    sgx_thread_mutex_lock(&global_mutex);
    sgx_status_t ret = g_aggData.get_wt1_and_sign_by_key(&g_aes_key, &g_priv_key, p_keys, key_size, p_wt1, out_size_wt1, p_wt1_mac, out_size_wt1_mac, p_sign, out_size_sign);
    sgx_thread_mutex_unlock(&global_mutex);

    return ret;
}



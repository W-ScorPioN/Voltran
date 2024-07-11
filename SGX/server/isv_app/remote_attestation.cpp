#include "remote_attestation.h"
#include <unistd.h>

#include "isv_enclave_u.h"

#include "sgx_uae_epid.h"
#include "sgx_uae_quote_ex.h"
#include "ecp.h"

#include "enclave_invoker.h"

#include "remote_attestation_result.h"

#ifndef SAFE_FREE
#define SAFE_FREE(ptr) {if (NULL != (ptr)) {free(ptr); (ptr) = NULL;}}
#endif

std::map<std::string, sgx_ra_context_t> remote_attestation::clientID2Context;
// std::mutex remote_attestation::m_Mutex_;

remote_attestation::remote_attestation(long cfd)
{
    client_sockfd = cfd;
}

remote_attestation::~remote_attestation()
{
}

sgx_ra_context_t remote_attestation::getRaContext()
{
    return context;
}

int remote_attestation::doAttestation(sgx_enclave_id_t enclave_id)
{
    FILE *OUTPUT = stdout;

    fprintf(OUTPUT, "\n >>> remote_attestation::doAttestation.\n");

    int ret = 0;

    {
        uint32_t extended_epid_group_id = 0;
        ret = sgx_get_extended_epid_group_id(&extended_epid_group_id);
        if (SGX_SUCCESS != ret)
        {
            // ret = -1;
            fprintf(OUTPUT, "\nError, call sgx_get_extended_epid_group_id fail [%s].", __FUNCTION__);
            return -1;
        }
        fprintf(OUTPUT, "\nCall sgx_get_extended_epid_group_id success.");

        p_msg0_full = (ra_samp_request_header_t*)malloc(sizeof(ra_samp_request_header_t) + sizeof(uint32_t));
        if (NULL == p_msg0_full)
        {
            return -1;
        }
        p_msg0_full->type = TYPE_RA_MSG0;
        p_msg0_full->size = sizeof(uint32_t);

        *(uint32_t*)((uint8_t*)p_msg0_full + sizeof(ra_samp_request_header_t)) = extended_epid_group_id;
        {
            fprintf(OUTPUT, "\nMSG0 body generated -\n");
            // PRINT_BYTE_ARRAY(OUTPUT, p_msg0_full->body, p_msg0_full->size);
        }
        // The ISV application sends msg0 to the SP.
        // The ISV decides whether to support this extended epid group id.
        fprintf(OUTPUT, "\nSending msg0 to remote attestation service provider.\n");

        ret = ra_network_send_receive2(client_sockfd, p_msg0_full, &p_msg0_resp_full);
        if (ret != 0)
        {
            fprintf(OUTPUT, "\nError, ra_network_send_receive for msg0 failed "
                "[%s].", __FUNCTION__);
            return -1;
        }
        fprintf(OUTPUT, "\nSent MSG0 to remote attestation service.\n");

        ret = sgx_select_att_key_id(p_msg0_resp_full->body, p_msg0_resp_full->size, &selected_key_id);
        if(SGX_SUCCESS != ret)
        {
            fprintf(OUTPUT, "\nInfo, call sgx_select_att_key_id fail, current platform configuration doesn't support this attestation key ID. [%s]",
                    __FUNCTION__);
            return -1;
        }
        fprintf(OUTPUT, "\nCall sgx_select_att_key_id success.");
    }

    {
        // ISV application creates the ISV enclave.
        do
        {
            ret = enclave_invoker::getInstance()->enclave_init_ra_t(enclave_id,
                                    &status,
                                    false,
                                    &context);
        //Ideally, this check would be around the full attestation flow.
        } while (SGX_ERROR_ENCLAVE_LOST == ret && enclave_lost_retry_time--);

        if(SGX_SUCCESS != ret || status)
        {
            fprintf(stdout, "\nError, call enclave_init_ra fail [%s]. ret = 0x%0x. status = 0x%0x, client_sockfd=%d", __FUNCTION__, ret, status, client_sockfd);
            return -1;
        }
        fprintf(OUTPUT, "\nCall enclave_init_ra success.context=%ld, client_sockfd=%d", context,client_sockfd);

        // isv application call uke sgx_ra_get_msg1
        p_msg1_full = (ra_samp_request_header_t*)malloc(sizeof(ra_samp_request_header_t) + sizeof(sgx_ra_msg1_t));
        if(NULL == p_msg1_full)
        {
            return -1;
        }
        p_msg1_full->type = TYPE_RA_MSG1;
        p_msg1_full->size = sizeof(sgx_ra_msg1_t);
        do
        {
            ret = enclave_invoker::getInstance()->sgx_ra_get_msg1_ex_t(&selected_key_id, context, enclave_id, sgx_ra_get_ga,
                                    (sgx_ra_msg1_t*)((uint8_t*)p_msg1_full + sizeof(ra_samp_request_header_t)));
            sleep(3); // Wait 3s between retries
        } while (SGX_ERROR_BUSY == ret && busy_retry_time--);
        if(SGX_SUCCESS != ret)
        {
            fprintf(stdout, "\nError, call sgx_ra_get_msg1_ex fail [%s]. ret = 0x%0x.context=%ld, client_sockfd=%d", __FUNCTION__, ret, context,client_sockfd);
            return -1;
        }
        else
        {
            fprintf(OUTPUT, "\nCall sgx_ra_get_msg1_ex success.\n");
            fprintf(OUTPUT, "\nMSG1 body generated -\n");
            // PRINT_BYTE_ARRAY(OUTPUT, p_msg1_full->body, p_msg1_full->size);
        }

        // The ISV application sends msg1 to the SP to get msg2,
        // msg2 needs to be freed when no longer needed.
        // The ISV decides whether to use linkable or unlinkable signatures.
        fprintf(OUTPUT, "\nSending msg1 to remote attestation service provider."
                        "Expecting msg2 back.\n");

        ret = ra_network_send_receive2(client_sockfd, p_msg1_full,  &p_msg2_full);

        if(ret != 0 || !p_msg2_full)
        {
            fprintf(OUTPUT, "\nError, ra_network_send_receive for msg1 failed "
                            "[%s].", __FUNCTION__);

            return -1;
        }
        else
        {
            // Successfully sent msg1 and received a msg2 back.
            // Time now to check msg2.
            if(TYPE_RA_MSG2 != p_msg2_full->type)
            {
                fprintf(OUTPUT, "\nError, didn't get MSG2 in response to MSG1. "
                                "[%s].", __FUNCTION__);
            }

            fprintf(OUTPUT, "\nSent MSG1 to remote attestation service "
                            "provider. Received the following MSG2:\n");
            // PRINT_BYTE_ARRAY(OUTPUT, p_msg2_full, (uint32_t)sizeof(ra_samp_response_header_t) + p_msg2_full->size);

            fprintf(OUTPUT, "\nA more descriptive representation of MSG2:\n");
            // PRINT_ATTESTATION_SERVICE_RESPONSE(OUTPUT, p_msg2_full);
        }


        sgx_ra_msg2_t* p_msg2_body = (sgx_ra_msg2_t*)((uint8_t*)p_msg2_full + sizeof(ra_samp_response_header_t));

        uint32_t msg3_size = 0;
        
        busy_retry_time = 2;
        // The ISV app now calls uKE sgx_ra_proc_msg2,
        // The ISV app is responsible for freeing the returned p_msg3!!
        do
        {
            ret = enclave_invoker::getInstance()->sgx_ra_proc_msg2_ex_t(&selected_key_id,
                                context,
                                enclave_id,
                                sgx_ra_proc_msg2_trusted,
                                sgx_ra_get_msg3_trusted,
                                p_msg2_body,
                                p_msg2_full->size,
                                &p_msg3,
                                &msg3_size);
        } while (SGX_ERROR_BUSY == ret && busy_retry_time--);
        if(!p_msg3)
        {
            fprintf(OUTPUT, "\nError, call sgx_ra_proc_msg2_ex fail. "
                            "p_msg3 = 0x%p,ret = 0x%0x [%s].", p_msg3,ret, __FUNCTION__);
            return -1;
        }
        if(SGX_SUCCESS != (sgx_status_t)ret)
        {
            fprintf(OUTPUT, "\nError, call sgx_ra_proc_msg2_ex fail. "
                            "ret = 0x%0x [%s].", ret, __FUNCTION__);
            return -1;
        }
        else
        {
            fprintf(OUTPUT, "\nCall sgx_ra_proc_msg2_ex success.\n");
            fprintf(OUTPUT, "\nMSG3 - \n");
        }

        // PRINT_BYTE_ARRAY(OUTPUT, p_msg3, msg3_size);

        p_msg3_full = (ra_samp_request_header_t*)malloc(sizeof(ra_samp_request_header_t) + msg3_size);
        if(NULL == p_msg3_full)
        {
            return -1;
        }
        p_msg3_full->type = TYPE_RA_MSG3;
        p_msg3_full->size = msg3_size;
        if(memcpy_s(p_msg3_full->body, msg3_size, p_msg3, msg3_size))
        {
            fprintf(OUTPUT,"\nError: INTERNAL ERROR - memcpy failed in [%s].", __FUNCTION__);
            return -1;
        }

        // The ISV application sends msg3 to the SP to get the attestation
        // result message, attestation result message needs to be freed when
        // no longer needed. The ISV service provider decides whether to use
        // linkable or unlinkable signatures. The format of the attestation
        // result is up to the service provider. This format is used for
        // demonstration.  Note that the attestation result message makes use
        // of both the MK for the MAC and the SK for the secret. These keys are
        // established from the SIGMA secure channel binding.
        ret = ra_network_send_receive2(client_sockfd, p_msg3_full, &p_att_result_msg_full);
        if(ret || !p_att_result_msg_full)
        {
            fprintf(OUTPUT, "\nError, sending msg3 failed [%s].", __FUNCTION__);
            return -1;
        }


        sample_ra_att_result_msg_t * p_att_result_msg_body =
            (sample_ra_att_result_msg_t *)((uint8_t*)p_att_result_msg_full + sizeof(ra_samp_response_header_t));
        if(TYPE_RA_ATT_RESULT != p_att_result_msg_full->type)
        {
            fprintf(OUTPUT, "\nError. Sent MSG3 successfully, but the message "
                            "received was NOT of type att_msg_result. Type = "
                            "%d. [%s].", p_att_result_msg_full->type,
                                __FUNCTION__);
            return -1;
        }
        else
        {
            fprintf(OUTPUT, "\nSent MSG3 successfully. Received an attestation result message back\n.");
        }

        fprintf(OUTPUT, "\nATTESTATION RESULT RECEIVED - ");
        // PRINT_BYTE_ARRAY(OUTPUT, p_att_result_msg_full->body, p_att_result_msg_full->size);

        // Check the MAC using MK on the attestation result message.
        // The format of the attestation result message is ISV specific.
        // This is a simple form for demonstration. In a real product,
        // the ISV may want to communicate more information.
        ret = verify_att_result_mac(enclave_id,
                &status,
                context,
                (uint8_t*)&p_att_result_msg_body->platform_info_blob,
                sizeof(ias_platform_info_blob_t),
                (uint8_t*)&p_att_result_msg_body->mac,
                sizeof(sgx_mac_t));
        if((SGX_SUCCESS != ret) ||
            (SGX_SUCCESS != status))
        {
            fprintf(OUTPUT, "\nError: INTEGRITY FAILED - attestation result message MK based cmac failed in [%s].", __FUNCTION__);
            return -1;
        }

        bool attestation_passed = true;
        // Check the attestation result for pass or fail.
        // Whether attestation passes or fails is a decision made by the ISV Server.
        // When the ISV server decides to trust the enclave, then it will return success.
        // When the ISV server decided to not trust the enclave, then it will return failure.
        if(0 != p_att_result_msg_full->status[0]
            || 0 != p_att_result_msg_full->status[1])
        {
            fprintf(OUTPUT, "\nError, attestation result message MK based cmac "
                            "failed in [%s].", __FUNCTION__);
            attestation_passed = false;
        }

        if(!attestation_passed)
        {
            return -1;
        }

        return 0;
    }
}

void remote_attestation::release()
{
    ra_free_network_response_buffer(p_msg0_resp_full);
    ra_free_network_response_buffer(p_msg2_full);
    ra_free_network_response_buffer(p_att_result_msg_full);

    // p_msg3 is malloc'd by the untrusted KE library. App needs to free.
    SAFE_FREE(p_msg3);
    SAFE_FREE(p_msg3_full);
    SAFE_FREE(p_msg1_full);
    SAFE_FREE(p_msg0_full);
}
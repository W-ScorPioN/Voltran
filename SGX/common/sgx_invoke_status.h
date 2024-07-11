#ifndef _SGX_INVOKE_ERROR_H_
#define _SGX_INVOKE_ERROR_H_

#define SGX_INVOKE_MK_ERROR(x)              (0x00000000|(x))

typedef enum _sgx_invoke_status_t
{
    SGX_INVOKE_SUCCESS                  = SGX_INVOKE_MK_ERROR(0x0000),
} sgx_invoke_status_t;

typedef enum _sgx_agg_status_t
{
    SGX_AGG_FINISH                  = SGX_INVOKE_MK_ERROR(0x0000),
    SGX_AGG_NOT_FINISH              = SGX_INVOKE_MK_ERROR(0x0001),
    SGX_MULT_AGG_FINISH             = SGX_INVOKE_MK_ERROR(0x0003),
    SGX_MULT_AGG_NOT_FINISH         = SGX_INVOKE_MK_ERROR(0x0004),

} sgx_agg_status_t;

#endif
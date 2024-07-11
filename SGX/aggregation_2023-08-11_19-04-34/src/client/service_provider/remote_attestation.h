#ifndef _REMOTE_ATTESTATION_H
#define _REMOTE_ATTESTATION_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "network_ra.h"
#include "ra_client.h"
#include "learning_data.h"

class remote_attestation 
{
public:
    remote_attestation();
    ~remote_attestation();

    int attest(sgx_client &client);

private:
    int startAttestation(sgx_client &client);
    int dealMessage0(sgx_client &client);
    int dealMessage1(sgx_client &client);
    int dealMessage3(sgx_client &client);
};

#endif
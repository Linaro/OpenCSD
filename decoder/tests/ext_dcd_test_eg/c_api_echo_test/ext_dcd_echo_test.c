/*
* \file       ext_dcd_echo_test_fact.c
* \brief      OpenCSD : Echo test custom decoder factory
*
* \copyright  Copyright (c) 2016, ARM Limited. All Rights Reserved.
*/

/*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_api/ocsd_c_api_types.h"
#include "ext_dcd_echo_test_fact.h"
#include "ext_dcd_echo_test.h"

/** The name of the decoder */
#define DECODER_NAME "ECHO_TEST"

/** Declare the trace data input function for the decoder - passed to library as call-back. */
static ocsd_datapath_resp_t echo_dcd_trace_data_in(const void *decoder_handle,
    const ocsd_datapath_op_t op,
    const ocsd_trc_index_t index,
    const uint32_t dataBlockSize,
    const uint8_t *pDataBlock,
    uint32_t *numBytesProcessed);

/** Allow library to update the packet monitor / sink in use flags to allow decoder to call CB as appropriate.*/
static void echo_dcd_update_mon_flags(const void *decoder_handle, const int flags);

/** init decoder on creation, along with library instance structure */
void echo_dcd_init(echo_decoder_t *decoder, ocsd_extern_dcd_inst_t *p_decoder_inst, const echo_dcd_cfg_t *p_config, const ocsd_extern_dcd_cb_fns *p_lib_callbacks)
{
    // initialise the decoder instance.

    // zero out the structure
    memset(decoder, 0, sizeof(echo_decoder_t));

    memcpy(&(decoder->reg_config), p_config, sizeof(echo_dcd_cfg_t));       // copy in the config structure.
    memcpy(&(decoder->lib_fns), p_lib_callbacks, sizeof(p_lib_callbacks));  // copy in the the library callbacks.


    // fill out the info to pass back to the library.

    // set up the decoder handle, name and CS Trace ID
    p_decoder_inst->decoder_handle = decoder;
    p_decoder_inst->p_decoder_name = DECODER_NAME;
    p_decoder_inst->cs_id = p_config->cs_id;

    // set up the data input callback
    p_decoder_inst->fn_data_in = echo_dcd_trace_data_in;
    p_decoder_inst->fn_update_pkt_mon = echo_dcd_update_mon_flags;
}

void echo_dcd_pkt_tostr(echo_dcd_pkt_t *pkt, char *buffer, const int buflen)
{
    snprintf(buffer, buflen, "ECHOTP[0x%02X] (0x%08X) \n", pkt->header, pkt->data);
}

void echo_dcd_update_mon_flags(const void *decoder_handle, const int flags)
{
    ((echo_decoder_t *)decoder_handle)->lib_fns.packetCBFlags = flags;
}

/**** Main decoder implementation ****/

ocsd_datapath_resp_t echo_dcd_trace_data_in(const void *decoder_handle,
    const ocsd_datapath_op_t op,
    const ocsd_trc_index_t index,
    const uint32_t dataBlockSize,
    const uint8_t *pDataBlock,
    uint32_t *numBytesProcessed)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    return resp;
}

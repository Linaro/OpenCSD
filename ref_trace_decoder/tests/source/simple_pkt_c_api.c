/*
 * \file       simple_pkt_c_api.c
 * \brief      Reference CoreSight Trace Decoder : 
 * 
 * \copyright  Copyright (c) 2015, ARM Limited. All Rights Reserved.
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

/*
 * Example of using the library with the C-API
 *
 * Simple test program to print packets from a single trace source stream.
 * Hard coded configuration based on the Juno r1-1 test snapshot.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "c_api/rctdl_c_api.h"
#include "etmv4/trc_pkt_types_etmv4.h"

/* path to juno snapshot, relative to tests/bin/<plat>/<dbg|rel> build output dir */
#ifdef _WIN32
const char *default_path_to_snapshot = "..\\..\\..\\snapshots\\juno_r1_1\\cstrace.bin";
#else
const char *default_path_to_snapshot = "../../../snapshots/juno_r1_1/cstrace.bin";
#endif

/* trace configuration structure - contains programmed register values of etmv4 hardware */
static rctdl_etmv4_cfg trace_config;

/* buffer to handle a packet string */
static char packet_str[1024];

/* Callback fuction to process the packets in the stream - simply print them out in this case */
rctdl_datapath_resp_t etm_v4i_packet_handler(const rctdl_datapath_op_t op, const rctdl_trc_index_t index_sop, const rctdl_etmv4_i_pkt *p_packet_in)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    switch(op)
    {
    default: break;
    case RCTDL_OP_DATA:
    case RCTDL_OP_EOT:
        printf("**** END OF TRACE ****\n");
    }

    return resp;
}

/* hard coded values from snapshot .ini files */
void set_config_struct()
{
    trace_config.arch_ver   = ARCH_V8;
    trace_config.core_prof  = profile_CortexA;

    trace_config.reg_configr    = 0x000000C1;
    trace_config.reg_traceidr   = 0x00000010;   /* this is the trace ID -> 0x10, change this to analyse other streams in snapshot.*/

    trace_config.reg_idr0   = 0x28000EA1;
    trace_config.reg_idr1   = 0x4100F403;
    trace_config.reg_idr2   = 0x00000488;
    trace_config.reg_idr8   = 0x0;
    trace_config.reg_idr9   = 0x0;
    trace_config.reg_idr10  = 0x0;
    trace_config.reg_idr11  = 0x0;
    trace_config.reg_idr12  = 0x0;
    trace_config.reg_idr13  = 0x0;
}

#define INPUT_BLOCK_SIZE 1024

/* process buffer until done or error */
rctdl_err_t process_data_block(dcd_tree_handle_t dcd_tree_h, int block_index, uint8_t *p_block, const int block_size)
{
    rctdl_err_t ret = RCTDL_OK;
    uint32_t bytes_done = 0;
    rctdl_datapath_resp_t dp_ret;
    uint32_t bytes_this_time = 0;

    while((bytes_done < block_size) && (ret == RCTDL_OK))
    {
        if(RCTDL_DATA_RESP_IS_CONT(dp_ret))
        {
            dp_ret = rctdl_dt_process_data(dcd_tree_h, 
                                RCTDL_OP_DATA,
                                block_index+bytes_done,
                                block_size-bytes_done,
                                ((uint8_t *)p_block)+bytes_done,
                                &bytes_this_time);
            bytes_done += bytes_this_time;
        }
        else if(RCTDL_DATA_RESP_IS_WAIT(dp_ret))
        {
            dp_ret = rctdl_dt_process_data(dcd_tree_h, RCTDL_OP_FLUSH,0,0,NULL,NULL);
        }
        else
            ret = RCTDL_ERR_DATA_DECODE_FATAL; /* data path responded with an error - stop processing */
    }
    return ret;
}

int process_trace_data(FILE *pf)
{
    rctdl_err_t ret = RCTDL_OK;
    dcd_tree_handle_t dcdtree_handle = C_API_INVALID_TREE_HANDLE;
    int data_done = 0;
    uint8_t data_buffer[INPUT_BLOCK_SIZE];
    rctdl_trc_index_t index = 0;
    size_t data_read;

    /*  source data is frame formatted, memory aligned from an ETR (no frame syncs) so create tree accordingly */
    dcdtree_handle = rctdl_create_dcd_tree(RCTDL_TRC_SRC_FRAME_FORMATTED, RCTDL_DFRMTR_FRAME_MEM_ALIGN);
    if(dcdtree_handle != C_API_INVALID_TREE_HANDLE)
    {
        /* populate the ETMv4 configuration structure */
        set_config_struct();

        /* create a packet processor for the ETM v4 configuration we have. */
        ret = rctdl_dt_create_etmv4i_pkt_proc(dcdtree_handle,&trace_config,&etm_v4i_packet_handler);

        /* now push the trace data through the packet processor */
        while(!feof(pf) && (ret == RCTDL_OK))
        {
            /* read from file */
            data_read = fread(data_buffer,1,INPUT_BLOCK_SIZE,pf);
            if(data_read > 0)
            {
                ret = process_data_block(dcdtree_handle, 
                                index,
                                data_buffer,
                                data_read);
                index += data_read;
            }
            else if(ferror(pf))
                ret = RCTDL_ERR_FILE_ERROR;
        }

        /* no errors - let the data path know we are at end of trace */
        if(ret == RCTDL_OK)
            rctdl_dt_process_data(dcdtree_handle, RCTDL_OP_EOT, 0,0,NULL,NULL);

        rctdl_destroy_dcd_tree(dcdtree_handle);
    }
    else
    {
        printf("Failed to create trace decode tree\n");
        ret = RCTDL_ERR_NOT_INIT;
    }
    return (int)ret;
}



int main(int argc, char *argv[])
{
    FILE *trace_data;
    char trace_file_path[256];
    int ret = 0;

    strcpy(trace_file_path,default_path_to_snapshot);
    trace_data = fopen(trace_file_path,"rb");
    if(trace_data != NULL)
    {
        ret = process_trace_data(trace_data);
        fclose(trace_data);
    }
    else
    {
        printf("Unable to open file %s to process trace data\n", trace_file_path);
        ret = -1;
    }
    return ret;
}
/* End of File simple_pkt_c_api.c */

/*
 * \file       simple_pkt_c_api.c
 * \brief      OpenCSD : 
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
 * Hard coded configuration based on the Juno r1-1 test snapshot for ETMv4 and
 * STM, TC2 test snapshot for ETMv3, PTM
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* include the C-API library header */
#include "c_api/opencsd_c_api.h"

/* path to juno snapshot, relative to tests/bin/<plat>/<dbg|rel> build output dir */
#ifdef _WIN32
const char *default_path_to_snapshot = "..\\..\\..\\snapshots\\juno_r1_1\\";
const char *tc2_snapshot = "..\\..\\..\\snapshots\\TC2\\";
#else
const char *default_path_to_snapshot = "../../../snapshots/juno_r1_1/";
const char *tc2_snapshot = "../../../snapshots/TC2/";
#endif

/* trace data and memory file dump names */
const char *trace_data_filename = "cstrace.bin";
const char *stmtrace_data_filename = "cstraceitm.bin";
const char *memory_dump_filename = "kernel_dump.bin";
const ocsd_vaddr_t mem_dump_address=0xFFFFFFC000081000;

static int using_mem_acc_cb = 0;
static int use_region_file = 0;

/* region list to test region file API */
file_mem_region_t region_list[4];

/* trace configuration structures - contains programmed register values of trace hardware */
static ocsd_etmv4_cfg trace_config;
static ocsd_etmv3_cfg trace_config_etmv3;
static ocsd_stm_cfg trace_config_stm;
static ocsd_ptm_cfg trace_config_ptm;

/* buffer to handle a packet string */
#define PACKET_STR_LEN 1024
static char packet_str[PACKET_STR_LEN];

/* decide if we decode & monitor, decode only or packet print */
typedef enum _test_op {
    TEST_PKT_PRINT,
    TEST_PKT_DECODE,
    TEST_PKT_DECODEONLY
} test_op_t;

static test_op_t op = TEST_PKT_PRINT;
static ocsd_trace_protocol_t test_protocol = OCSD_PROTOCOL_ETMV4I;
static uint8_t test_trc_id_override = 0x00;

static int print_data_array(const uint8_t *p_array, const int array_size, char *p_buffer, int buf_size)
{
    int chars_printed = 0;
    int bytes_processed;
    p_buffer[0] = 0;
    
    if(buf_size > 9)
    {
        /* set up the header */
        strcat(p_buffer,"[ ");
        chars_printed+=2;

        for(bytes_processed = 0; bytes_processed < array_size; bytes_processed++)
        {
           sprintf(p_buffer+chars_printed,"0x%02X ", p_array[bytes_processed]);
           chars_printed += 5;
           if((chars_printed + 5) > buf_size)
               break;
        }

        strcat(p_buffer,"];");
        chars_printed+=2;
    }
    else if(buf_size >= 4)
    {
        sprintf(p_buffer,"[];");
        chars_printed+=3;
    }
    return chars_printed;
}

/*  choose the operation to use for the test. */
static void process_cmd_line(int argc, char *argv[])
{
    int idx = 1;

    while(idx < argc)
    {
        if(strcmp(argv[idx],"-decode_only") == 0)
        {
            op = TEST_PKT_DECODEONLY;
        }
        else if(strcmp(argv[idx],"-decode") == 0)
        {
            op = TEST_PKT_DECODE;
        }
        else if(strcmp(argv[idx],"-id") == 0)
        {
            idx++;
            if(idx < argc)
            {
                test_trc_id_override = (uint8_t)(strtoul(argv[idx],0,0));
                printf("ID override = 0x%02X\n",test_trc_id_override);
            }
        }
        else if(strcmp(argv[idx],"-etmv3") == 0)
        {
            test_protocol =  OCSD_PROTOCOL_ETMV3;
            default_path_to_snapshot = tc2_snapshot;
        }
        else if(strcmp(argv[idx],"-ptm") == 0)
        {
            test_protocol =  OCSD_PROTOCOL_PTM;
            default_path_to_snapshot = tc2_snapshot;
        }
        else if(strcmp(argv[idx],"-stm") == 0)
        {
            test_protocol = OCSD_PROTOCOL_STM;
            trace_data_filename = stmtrace_data_filename;
        }
        else if(strcmp(argv[idx],"-test_cb") == 0)
        {
            using_mem_acc_cb = 1;
        }
        else if(strcmp(argv[idx],"-test_region_file") == 0)
        {
            use_region_file = 1;
        }
        else 
            printf("Ignored unknown argument %s\n", argv[idx]);
        idx++;
    }
}

/* Callback function to process the packets in the stream - 
   simply print them out in this case 
 */
ocsd_datapath_resp_t packet_handler(const ocsd_datapath_op_t op, const ocsd_trc_index_t index_sop, const void *p_packet_in)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    int offset = 0;

    switch(op)
    {
    default: break;
    case OCSD_OP_DATA:
        sprintf(packet_str,"Idx:%ld; ", index_sop);
        offset = strlen(packet_str);
   
        /* got a packet - convert to string and use the libraries' message output to print to file and stdoout */
        if(ocsd_pkt_str(test_protocol,p_packet_in,packet_str+offset,PACKET_STR_LEN-offset) == OCSD_OK)
        {
            /* add in <CR> */
            if(strlen(packet_str) == PACKET_STR_LEN - 1) /* maximum length */
                packet_str[PACKET_STR_LEN-2] = '\n';
            else
                strcat(packet_str,"\n");

            /* print it using the library output logger. */
            ocsd_def_errlog_msgout(packet_str);
        }
        else
            resp = OCSD_RESP_FATAL_INVALID_PARAM;  /* mark fatal error */
        break;

    case OCSD_OP_EOT:
        sprintf(packet_str,"**** END OF TRACE ****\n");
        ocsd_def_errlog_msgout(packet_str);
        break;
    }

    return resp;
}

/* decode memory access using a CB function 
* tests CB API and add / remove mem acc API.
*/
static FILE *dump_file = NULL;
static ocsd_mem_space_acc_t dump_file_mem_space = OCSD_MEM_SPACE_ANY;
static long mem_file_size = 0;
static ocsd_vaddr_t mem_file_en_address = 0;  /* end address last inclusive address in file. */

static uint32_t  mem_acc_cb(const void *p_context, const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint32_t reqBytes, uint8_t *byteBuffer)
{
    uint32_t read_bytes = 0;
    size_t file_read_bytes;

    if(dump_file == NULL)
        return 0;
    
    /* bitwise & the incoming mem space and supported mem space to confirm coverage */
    if(((uint8_t)mem_space & (uint8_t)dump_file_mem_space ) == 0)   
        return 0;

    /* calculate the bytes that can be read */
    if((address >= mem_dump_address) && (address <= mem_file_en_address))
    {
        /* some bytes in our range */
        read_bytes = reqBytes;

        if((address + reqBytes - 1) > mem_file_en_address)
        {
            /* more than are available - just read the available */
            read_bytes = (uint32_t)(mem_file_en_address - (address - 1));
        }
    }

    /* read some bytes if more than 0 to read. */ 
    if(read_bytes != 0)
    {
        fseek(dump_file,(long)(address-mem_dump_address),SEEK_SET);
        file_read_bytes = fread(byteBuffer,sizeof(uint8_t),read_bytes,dump_file);
        if(file_read_bytes < read_bytes)
            read_bytes = file_read_bytes;
    }
    return read_bytes;
}

static ocsd_err_t create_mem_acc_cb(dcd_tree_handle_t dcd_tree_h, const char *mem_file_path)
{
    ocsd_err_t err = OCSD_OK;
    dump_file = fopen(mem_file_path,"rb");
    if(dump_file != NULL)
    {
        fseek(dump_file,0,SEEK_END);
        mem_file_size = ftell(dump_file);
        mem_file_en_address = mem_dump_address + mem_file_size - 1;

        err = ocsd_dt_add_callback_mem_acc(dcd_tree_h,
            mem_dump_address,mem_file_en_address,dump_file_mem_space,&mem_acc_cb,0);
        if(err != OCSD_OK)
        {
            fclose(dump_file);
            dump_file = NULL;
        }            
    }
    else
        err = OCSD_ERR_MEM_ACC_FILE_NOT_FOUND;
    return err;
}

static void destroy_mem_acc_cb(dcd_tree_handle_t dcd_tree_h)
{
    if(dump_file != NULL)
    {
        ocsd_dt_remove_mem_acc(dcd_tree_h,mem_dump_address,dump_file_mem_space);
        fclose(dump_file);
        dump_file = NULL;
    }
}

void packet_monitor(const ocsd_datapath_op_t op, 
                              const ocsd_trc_index_t index_sop, 
                              const void *p_packet_in,
                              const uint32_t size,
                              const uint8_t *p_data)
{
    int offset = 0;

    switch(op)
    {
    default: break;
    case OCSD_OP_DATA:
        sprintf(packet_str,"Idx:%ld;", index_sop);
        offset = strlen(packet_str);
        offset+= print_data_array(p_data,size,packet_str+offset,PACKET_STR_LEN-offset);

        /* got a packet - convert to string and use the libraries' message output to print to file and stdoout */
        if(ocsd_pkt_str(test_protocol,p_packet_in,packet_str+offset,PACKET_STR_LEN-offset) == OCSD_OK)
        {
            /* add in <CR> */
            if(strlen(packet_str) == PACKET_STR_LEN - 1) /* maximum length */
                packet_str[PACKET_STR_LEN-2] = '\n';
            else
                strcat(packet_str,"\n");

            /* print it using the library output logger. */
            ocsd_def_errlog_msgout(packet_str);
        }
        break;

    case OCSD_OP_EOT:
        sprintf(packet_str,"**** END OF TRACE ****\n");
        ocsd_def_errlog_msgout(packet_str);
        break;
    }
}

ocsd_datapath_resp_t gen_trace_elem_print(const void *p_context, const ocsd_trc_index_t index_sop, const uint8_t trc_chan_id, const ocsd_generic_trace_elem *elem)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    int offset = 0;

    sprintf(packet_str,"Idx:%ld; TrcID:0x%02X; ", index_sop, trc_chan_id);
    offset = strlen(packet_str);

    if(ocsd_gen_elem_str(elem, packet_str+offset,PACKET_STR_LEN - offset) == OCSD_OK)
    {
        /* add in <CR> */
        if(strlen(packet_str) == PACKET_STR_LEN - 1) /* maximum length */
            packet_str[PACKET_STR_LEN-2] = '\n';
        else
            strcat(packet_str,"\n");
    }
    else
    {
        strcat(packet_str,"Unable to create element string\n");
    }

    /* print it using the library output logger. */
    ocsd_def_errlog_msgout(packet_str);

    return resp;
}

/************************************************************************/
/*** ETMV4 ***/
/************************************************************************/

/* hard coded values from snapshot .ini files */
void set_config_struct_etmv4()
{
    trace_config.arch_ver   = ARCH_V8;
    trace_config.core_prof  = profile_CortexA;

    trace_config.reg_configr    = 0x000000C1;
    trace_config.reg_traceidr   = 0x00000010;   /* this is the trace ID -> 0x10, change this to analyse other streams in snapshot.*/

    if(test_trc_id_override != 0)
    {
        trace_config.reg_traceidr = (uint32_t)test_trc_id_override;
    }

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

ocsd_datapath_resp_t etm_v4i_packet_handler(const void *p_context, const ocsd_datapath_op_t op, const ocsd_trc_index_t index_sop, const ocsd_etmv4_i_pkt *p_packet_in)
{
    return packet_handler(op,index_sop,(const void *)p_packet_in);
}


void etm_v4i_packet_monitor(  const void *p_context,
                              const ocsd_datapath_op_t op, 
                              const ocsd_trc_index_t index_sop, 
                              const ocsd_etmv4_i_pkt *p_packet_in,
                              const uint32_t size,
                              const uint8_t *p_data)
{
    packet_monitor(op,index_sop,(void *)p_packet_in,size,p_data);
}

static ocsd_err_t create_decoder_etmv4(dcd_tree_handle_t dcd_tree_h)
{
    ocsd_err_t ret = OCSD_OK;
    char mem_file_path[512];
    int i;
    uint32_t i0adjust = 0x100;

    /* populate the ETMv4 configuration structure */
    set_config_struct_etmv4();

    if(op == TEST_PKT_PRINT) /* packet printing only */
    {
        /* Create a packet processor on the decode tree for the ETM v4 configuration we have. 
            We need to supply the configuration, and a packet handling callback.
        */
        ret = ocsd_dt_create_etmv4i_pkt_proc(dcd_tree_h,&trace_config,&etm_v4i_packet_handler,0);
    }
    else
    {
        /* Full decode - need decoder, and memory dump */

        /* create the packet decoder and packet processor pair */
        ret = ocsd_dt_create_etmv4i_decoder(dcd_tree_h,&trace_config);
        if(ret == OCSD_OK)
        {
            if((op != TEST_PKT_DECODEONLY) && (ret == OCSD_OK))
            {
                    /* print the packets as well as the decode. */
                    ret = ocsd_dt_attach_etmv4i_pkt_mon(dcd_tree_h, (uint8_t)(trace_config.reg_traceidr & 0xFF), etm_v4i_packet_monitor,0);
            }
        }

        /* trace data file path */
        strcpy(mem_file_path,default_path_to_snapshot);
        strcat(mem_file_path,memory_dump_filename);

        if(ret == OCSD_OK)
        {
            if(using_mem_acc_cb)
            {
                ret = create_mem_acc_cb(dcd_tree_h,mem_file_path);
            }
            else
            {
                if(use_region_file)
                {
                    dump_file = fopen(mem_file_path,"rb");
                    if(dump_file != NULL)
                    {
                        fseek(dump_file,0,SEEK_END);
                        mem_file_size = ftell(dump_file);
                        fclose(dump_file);
                        

                        /* populate the region list - split existing file into four regions */
                        for(i = 0; i < 4; i++)
                        {
                            if(i != 0)
                                i0adjust = 0;
                            region_list[i].start_address = mem_dump_address + (i *  mem_file_size/4) + i0adjust;
                            region_list[i].region_size = (mem_file_size/4) - i0adjust;
                            region_list[i].file_offset = (i * mem_file_size/4) +  i0adjust;
                        }

                        /* create a memory file accessor - full binary file */
                        ret = ocsd_dt_add_binfile_region_mem_acc(dcd_tree_h,&region_list[0],4,OCSD_MEM_SPACE_ANY,mem_file_path);

                    }
                    else 
                        ret  = OCSD_ERR_MEM_ACC_FILE_NOT_FOUND;
                }
                else
                {
                    /* create a memory file accessor - full binary file */
                    ret = ocsd_dt_add_binfile_mem_acc(dcd_tree_h,mem_dump_address,OCSD_MEM_SPACE_ANY,mem_file_path);
                }
            }
        }
    }
    return ret;
}

/************************************************************************/
/*** ETMV3 ***/
/************************************************************************/

static void set_config_struct_etmv3()
{
    trace_config_etmv3.arch_ver = ARCH_V7;
    trace_config_etmv3.core_prof = profile_CortexA;
    trace_config_etmv3.reg_ccer  = 0x344008F2;
    trace_config_etmv3.reg_ctrl  = 0x10001860;
    trace_config_etmv3.reg_idr  = 0x410CF250;
    trace_config_etmv3.reg_trc_id  = 0x010;
    if(test_trc_id_override != 0)
    {
        trace_config_etmv3.reg_trc_id = (uint32_t)test_trc_id_override;
    }
}

ocsd_datapath_resp_t etm_v3_packet_handler(const void *p_context, const ocsd_datapath_op_t op, const ocsd_trc_index_t index_sop, const ocsd_etmv3_pkt *p_packet_in)
{
    return packet_handler(op,index_sop,(const void *)p_packet_in);
}

void etm_v3_packet_monitor(   const ocsd_datapath_op_t op, 
                              const ocsd_trc_index_t index_sop, 
                              const ocsd_etmv3_pkt *p_packet_in,
                              const uint32_t size,
                              const uint8_t *p_data)
{
    packet_monitor(op,index_sop,(void *)p_packet_in,size,p_data);
}

static ocsd_err_t create_decoder_etmv3(dcd_tree_handle_t dcd_tree_h)
{
    ocsd_err_t ret = OCSD_OK;
    /*char mem_file_path[512];*/

    /* populate the ETMv3 configuration structure */
    set_config_struct_etmv3();


    if(op == TEST_PKT_PRINT) /* packet printing only */
    {
        /* Create a packet processor on the decode tree for the ETM v4 configuration we have. 
            We need to supply the configuration, and a packet handling callback.
        */
        ret = ocsd_dt_create_etmv3_pkt_proc(dcd_tree_h,&trace_config_etmv3,&etm_v3_packet_handler,0);
    }
    else
    {
        /* Full decode - need decoder, and memory dump */
        /* not supported in library at present */
        
#if 0
        /* create the packet decoder and packet processor pair */
        ret = ocsd_dt_create_etmv3_decoder(dcd_tree_h,&trace_config_etmv3);
        if(ret == OCSD_OK)
        {
            if((op != TEST_PKT_DECODEONLY) && (ret == OCSD_OK))
            {
                    ret = ocsd_dt_attach_etmv3_pkt_mon(dcd_tree_h, (uint8_t)(trace_config_etmv3.reg_trc_id & 0x7F), etm_v4i_packet_monitor);
            }
        }

        /* trace data file path */
        strcpy(mem_file_path,tc2_snapshot);
        strcat(mem_file_path,memory_dump_filename);

        if(ret == OCSD_OK)
        {
            /* create a memory file accessor */
            ret = ocsd_dt_add_binfile_mem_acc(dcd_tree_h,mem_dump_address,OCSD_MEM_SPACE_ANY,mem_file_path);
        }
#else
        printf("ETMv3 Full decode not supported in library at present. Packet print only\n");
        ret = OCSD_ERR_RDR_NO_DECODER;
#endif
    }
    return ret;
}

/************************************************************************/
/*** PTM ***/
/************************************************************************/
/* hard coded values from snapshot .ini files */
void set_config_struct_ptm()
{
    trace_config_ptm.arch_ver = ARCH_V7;
    trace_config_ptm.core_prof = profile_CortexA;
    trace_config_ptm.reg_ccer  = 0x34C01AC2;
    trace_config_ptm.reg_ctrl  = 0x10001000;
    trace_config_ptm.reg_idr  = 0x411CF312;
    trace_config_ptm.reg_trc_id  = 0x013;
    if(test_trc_id_override != 0)
    {
        trace_config_ptm.reg_trc_id = (uint32_t)test_trc_id_override;
    }
}

ocsd_datapath_resp_t ptm_packet_handler(const void *p_context, const ocsd_datapath_op_t op, const ocsd_trc_index_t index_sop, const ocsd_ptm_pkt *p_packet_in)
{
    return packet_handler(op,index_sop,(const void *)p_packet_in);
}

void ptm_packet_monitor(  const void *p_context,
                              const ocsd_datapath_op_t op, 
                              const ocsd_trc_index_t index_sop, 
                              const ocsd_ptm_pkt *p_packet_in,
                              const uint32_t size,
                              const uint8_t *p_data)
{
    packet_monitor(op,index_sop,(void *)p_packet_in,size,p_data);
}

static ocsd_err_t create_decoder_ptm(dcd_tree_handle_t dcd_tree_h)
{
    ocsd_err_t ret = OCSD_OK;
    char mem_file_path[512];
    int i;
    uint32_t i0adjust = 0x100;

    /* populate the PTM configuration structure */
    set_config_struct_ptm();

    if(op == TEST_PKT_PRINT) /* packet printing only */
    {
        /* Create a packet processor on the decode tree for the PTM  configuration we have. 
            We need to supply the configuration, and a packet handling callback.
        */
        ret = ocsd_dt_create_ptm_pkt_proc(dcd_tree_h,&trace_config_ptm,&ptm_packet_handler,0);
    }
    else
    {
        /* Full decode - need decoder, and memory dump */

        /* create the packet decoder and packet processor pair */
        ret = ocsd_dt_create_ptm_decoder(dcd_tree_h,&trace_config_ptm);
        if(ret == OCSD_OK)
        {
            if((op != TEST_PKT_DECODEONLY) && (ret == OCSD_OK))
            {
                    /* print the packets as well as the decode. */
                ret = ocsd_dt_attach_ptm_pkt_mon(dcd_tree_h, (uint8_t)(trace_config_ptm.reg_trc_id & 0xFF), ptm_packet_monitor,0);
            }
        }

        /* trace data file path */
        strcpy(mem_file_path,default_path_to_snapshot);
        strcat(mem_file_path,memory_dump_filename);

        if(ret == OCSD_OK)
        {
            if(using_mem_acc_cb)
            {
                ret = create_mem_acc_cb(dcd_tree_h,mem_file_path);
            }
            else
            {
                if(use_region_file)
                {
                    dump_file = fopen(mem_file_path,"rb");
                    if(dump_file != NULL)
                    {
                        fseek(dump_file,0,SEEK_END);
                        mem_file_size = ftell(dump_file);
                        fclose(dump_file);
                        

                        /* populate the region list - split existing file into four regions */
                        for(i = 0; i < 4; i++)
                        {
                            if(i != 0)
                                i0adjust = 0;
                            region_list[i].start_address = mem_dump_address + (i *  mem_file_size/4) + i0adjust;
                            region_list[i].region_size = (mem_file_size/4) - i0adjust;
                            region_list[i].file_offset = (i * mem_file_size/4) +  i0adjust;
                        }

                        /* create a memory file accessor - full binary file */
                        ret = ocsd_dt_add_binfile_region_mem_acc(dcd_tree_h,&region_list[0],4,OCSD_MEM_SPACE_ANY,mem_file_path);

                    }
                    else 
                        ret  = OCSD_ERR_MEM_ACC_FILE_NOT_FOUND;
                }
                else
                {
                    /* create a memory file accessor - full binary file */
                    ret = ocsd_dt_add_binfile_mem_acc(dcd_tree_h,mem_dump_address,OCSD_MEM_SPACE_ANY,mem_file_path);
                }
            }
        }
    }
    return ret;
}


/************************************************************************/
/*** STM ***/
/************************************************************************/

#define STMTCSR_TRC_ID_MASK     0x007F0000
#define STMTCSR_TRC_ID_SHIFT    16
static void set_config_struct_stm()
{
    trace_config_stm.reg_tcsr = 0x00A00005;
    if(test_trc_id_override != 0)
    {
        trace_config_stm.reg_tcsr &= ~STMTCSR_TRC_ID_MASK;
        trace_config_stm.reg_tcsr |= ((((uint32_t)test_trc_id_override) << STMTCSR_TRC_ID_SHIFT) & STMTCSR_TRC_ID_MASK);
    }
    trace_config_stm.reg_feat3r = 0x10000;  /* channel default */
    trace_config_stm.reg_devid = 0xFF;      /* master default */

    /* not using hw event trace decode */
    trace_config_stm.reg_hwev_mast = 0;
    trace_config_stm.reg_feat1r = 0;
    trace_config_stm.hw_event = HwEvent_Unknown_Disabled;
}

ocsd_datapath_resp_t stm_packet_handler(const void *p_context, const ocsd_datapath_op_t op, const ocsd_trc_index_t index_sop, const ocsd_stm_pkt *p_packet_in)
{
    return packet_handler(op,index_sop,(const void *)p_packet_in);
}

void stm_packet_monitor(  const ocsd_datapath_op_t op, 
                              const ocsd_trc_index_t index_sop, 
                              const ocsd_stm_pkt *p_packet_in,
                              const uint32_t size,
                              const uint8_t *p_data)
{
    packet_monitor(op,index_sop,(void *)p_packet_in,size,p_data);
}

static ocsd_err_t create_decoder_stm(dcd_tree_handle_t dcd_tree_h)
{
    ocsd_err_t ret = OCSD_OK;
    /*char mem_file_path[512];*/

    /* populate the STM configuration structure */
    set_config_struct_stm();


    if(op == TEST_PKT_PRINT) /* packet printing only */
    {
        /* Create a packet processor on the decode tree for the ETM v4 configuration we have. 
            We need to supply the configuration, and a packet handling callback.
        */
        ret = ocsd_dt_create_stm_pkt_proc(dcd_tree_h,&trace_config_stm,&stm_packet_handler,0);
    }
    else
    {
        /* Full decode */
        /* not supported in library at present */
        
#if 0
        /* create the packet decoder and packet processor pair */
        ret = ocsd_dt_create_stm_decoder(dcd_tree_h,&trace_config_stm);
        if(ret == OCSD_OK)
        {
            if((op != TEST_PKT_DECODEONLY) && (ret == OCSD_OK))
            {
                    ret = ocsd_dt_attach_stm_pkt_mon(dcd_tree_h, (uint8_t)((trace_config_stm.reg_tcsr & STMTCSR_TRC_ID_MASK) >> STMTCSR_TRC_ID_SHIFT), stm_packet_monitor);
            }
        }

        /* trace data file path */
        strcpy(mem_file_path,);
        strcat(mem_file_path,memory_dump_filename);

        if(ret == OCSD_OK)
        {
            /* create a memory file accessor */
            ret = ocsd_dt_add_binfile_mem_acc(dcd_tree_h,mem_dump_address,OCSD_MEM_SPACE_ANY,mem_file_path);
        }
#else
        printf("STM Full decode not supported in library at present. Packet print only\n");
        ret = OCSD_ERR_RDR_NO_DECODER;
#endif
    }
    return ret;
}



/************************************************************************/

/* create a decoder according to options */
static ocsd_err_t create_decoder(dcd_tree_handle_t dcd_tree_h)
{
    ocsd_err_t err = OCSD_OK;
    switch(test_protocol)
    {
    case OCSD_PROTOCOL_ETMV4I:
        err = create_decoder_etmv4(dcd_tree_h);
        break;

    case OCSD_PROTOCOL_ETMV3:
        err = create_decoder_etmv3(dcd_tree_h);
        break;

    case OCSD_PROTOCOL_STM:
        err = create_decoder_stm(dcd_tree_h);
        break;

    case OCSD_PROTOCOL_PTM:
        err = create_decoder_ptm(dcd_tree_h);
        break;

    default:
        err = OCSD_ERR_NO_PROTOCOL;
        break;
    }
    return err;
}

#define INPUT_BLOCK_SIZE 1024

/* process buffer until done or error */
ocsd_err_t process_data_block(dcd_tree_handle_t dcd_tree_h, int block_index, uint8_t *p_block, const int block_size)
{
    ocsd_err_t ret = OCSD_OK;
    uint32_t bytes_done = 0;
    ocsd_datapath_resp_t dp_ret = OCSD_RESP_CONT;
    uint32_t bytes_this_time = 0;

    while((bytes_done < (uint32_t)block_size) && (ret == OCSD_OK))
    {
        if(OCSD_DATA_RESP_IS_CONT(dp_ret))
        {
            dp_ret = ocsd_dt_process_data(dcd_tree_h, 
                                OCSD_OP_DATA,
                                block_index+bytes_done,
                                block_size-bytes_done,
                                ((uint8_t *)p_block)+bytes_done,
                                &bytes_this_time);
            bytes_done += bytes_this_time;
        }
        else if(OCSD_DATA_RESP_IS_WAIT(dp_ret))
        {
            dp_ret = ocsd_dt_process_data(dcd_tree_h, OCSD_OP_FLUSH,0,0,NULL,NULL);
        }
        else
            ret = OCSD_ERR_DATA_DECODE_FATAL; /* data path responded with an error - stop processing */
    }
    return ret;
}

int process_trace_data(FILE *pf)
{
    ocsd_err_t ret = OCSD_OK;
    dcd_tree_handle_t dcdtree_handle = C_API_INVALID_TREE_HANDLE;
    uint8_t data_buffer[INPUT_BLOCK_SIZE];
    ocsd_trc_index_t index = 0;
    size_t data_read;


    /*  Create a decode tree for this source data.
        source data is frame formatted, memory aligned from an ETR (no frame syncs) so create tree accordingly 
    */
    dcdtree_handle = ocsd_create_dcd_tree(OCSD_TRC_SRC_FRAME_FORMATTED, OCSD_DFRMTR_FRAME_MEM_ALIGN);

    if(dcdtree_handle != C_API_INVALID_TREE_HANDLE)
    {

        ret = create_decoder(dcdtree_handle);
        ocsd_tl_log_mapped_mem_ranges(dcdtree_handle);

        if(ret == OCSD_OK)
            /* attach the generic trace element output callback */
            ret = ocsd_dt_set_gen_elem_outfn(dcdtree_handle,gen_trace_elem_print,0);

        /* now push the trace data through the packet processor */
        while(!feof(pf) && (ret == OCSD_OK))
        {
            /* read from file */
            data_read = fread(data_buffer,1,INPUT_BLOCK_SIZE,pf);
            if(data_read > 0)
            {
                /* process a block of data - any packets from the trace stream 
                   we have configured will appear at the callback 
                */
                ret = process_data_block(dcdtree_handle, 
                                index,
                                data_buffer,
                                data_read);
                index += data_read;
            }
            else if(ferror(pf))
                ret = OCSD_ERR_FILE_ERROR;
        }

        /* no errors - let the data path know we are at end of trace */
        if(ret == OCSD_OK)
            ocsd_dt_process_data(dcdtree_handle, OCSD_OP_EOT, 0,0,NULL,NULL);


        /* shut down the mem acc CB if in use. */
        if(using_mem_acc_cb)
        {
            destroy_mem_acc_cb(dcdtree_handle);
        }

        /* dispose of the decode tree - which will dispose of any packet processors we created 
        */
        ocsd_destroy_dcd_tree(dcdtree_handle);
    }
    else
    {
        printf("Failed to create trace decode tree\n");
        ret = OCSD_ERR_NOT_INIT;
    }
    return (int)ret;
}

int main(int argc, char *argv[])
{
    FILE *trace_data;
    char trace_file_path[512];
    int ret = 0;
    char message[512];

    /* command line params */
    process_cmd_line(argc,argv);
    
    /* trace data file path */
    strcpy(trace_file_path,default_path_to_snapshot);
    strcat(trace_file_path,trace_data_filename);
    trace_data = fopen(trace_file_path,"rb");

    if(trace_data != NULL)
    {
        /* set up the logging in the library - enable the error logger, with an output printer*/
        ret = ocsd_def_errlog_init(OCSD_ERR_SEV_INFO,1);
        
        /* set up the output - to file and stdout, set custom logfile name */
        if(ret == 0)
            ret = ocsd_def_errlog_config_output(C_API_MSGLOGOUT_FLG_FILE | C_API_MSGLOGOUT_FLG_STDOUT, "c_api_test.log");

        /* print sign-on message in log */
        sprintf(message, "C-API packet print test\nLibrary Version %s\n\n",ocsd_get_version_str());
        ocsd_def_errlog_msgout(message);
        

        /* process the trace data */
        if(ret == 0)
            ret = process_trace_data(trace_data);

        /* close the data file */
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

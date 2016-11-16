/*
 * \file       ocsd_c_api_custom.h
 * \brief      OpenCSD : Custom decoder interface types and structures
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
#ifndef ARM_OCSD_C_API_CUSTOM_H_INCLUDED
#define ARM_OCSD_C_API_CUSTOM_H_INCLUDED

#include "ocsd_c_api_types.h"

/* Custom decoder C-API interface types. */

/* Raw trace data input function - a decoder must have one of these */
typedef ocsd_datapath_resp_t (* fnTraceDataIn)( const void *decoder_handle, 
                                                const ocsd_datapath_op_t op,
                                                const ocsd_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed);

/* function to update the in-use flags for the packet sinks - must be implemented in the decoder */
typedef void (* fnUpdatePktMonFlags)(const void *decoder_handle, const int flags);


/* callback function to connect into the generic element output point */
typedef ocsd_datapath_resp_t (* fnGenElemOpCB)( const void *lib_context,
                                                const ocsd_trc_index_t index_sop, 
                                                const uint8_t trc_chan_id, 
                                                const ocsd_generic_trace_elem *elem); 
 
/* callback functions to connect into the library error logging mechanism */
typedef void (* fnLogErrorCB)(  const void *lib_context, 
                                const ocsd_err_severity_t filter_level, 
                                const ocsd_err_t code, 
                                const ocsd_trc_index_t idx, 
                                const uint8_t chan_id, 
                                const char *pMsg);

typedef void (* fnLogMsgCB)(const void *lib_context, const ocsd_err_severity_t filter_level, const char *msg);

/* callback function to connect an ARM instruction decoder */
typedef ocsd_err_t (* fnDecodeArmInstCB)(const void *lib_context, ocsd_instr_info *instr_info);

/* callback function to connect the memory accessor interface */
typedef ocsd_err_t (* fnMemAccessCB)(const void *lib_context,
                                     const ocsd_vaddr_t address, 
                                     const uint8_t cs_trace_id, 
                                     const ocsd_mem_space_acc_t mem_space, 
                                     uint32_t *num_bytes, 
                                     uint8_t *p_buffer);

/* callback function to connect to the packet monitor interface of the packet processor */
typedef void (* fnPktMonCB)(  const void *lib_context,
                              const ocsd_datapath_op_t op,
                              const ocsd_trc_index_t index_sop,
                              const void *pkt,
                              const uint32_t size,
                              const uint8_t *p_data);

/* callback function to connect to the packet sink interface, on the main decode 
    data path - use if decoder created as packet processor only */
typedef ocsd_datapath_resp_t (* fnPktDataSinkCB)( const void *lib_context,
                                                  const ocsd_datapath_op_t op,
                                                  const ocsd_trc_index_t index_sop,
                                                  const void *pkt);

/** an instance of this is owned by the decoder, filled in by the library - allows the CB fns in the library decode tree to be called. */
typedef struct _ocsd_extern_dcd_cb_fns {
/* Callback functions */
    fnGenElemOpCB       fn_gen_elem_out;            /**< Callback to output a generic element. */
    fnLogErrorCB        fn_log_error;               /**< Callback to output an error.  */
    fnLogMsgCB          fn_log_msg;                 /**< Callback to output a message. */
    fnDecodeArmInstCB   fn_arm_instruction_decode;  /**< Callback to decode an ARM instruction. */
    fnMemAccessCB       fn_memory_access;           /**< Callback to access memory images related to the trace capture. */
    fnPktMonCB          fn_packet_mon;              /**< Callback to output trace packet to packet monitor. */
    fnPktDataSinkCB     fn_packet_data_sink;        /**< Callback to output trace packet to packet sink - if in pack processing only mode. */
/* CB in use flags. */
    int packetCBFlags;  /**< Flags to indicate if the packet sink / packet monitor callbacks are in use. ( OCSD_CUST_DCD_PKT_CB_USE_MON / OCSD_CUST_DCD_PKT_CB_USE_SINK) */
/* library context */
    const void *lib_context;  /** library context pointer - use in callbacks to allow the library to load the correct context data. */
} ocsd_extern_dcd_cb_fns;

/** Flag to indicate the the packet monitor is in use in the library */
#define OCSD_CUST_DCD_PKT_CB_USE_MON  0x1
/** Flag to indicate the the packet sink is in use in the library */
#define OCSD_CUST_DCD_PKT_CB_USE_SINK 0x2

/** Owned by the library instance object, this structure is filled in by the ocsd_extern_dcd_fact_t createDecoder() function. */
typedef struct _ocsd_extern_dcd_inst {
    /* Mandatory decoder call back functions - library initialisation will fail without these. */
    fnTraceDataIn       fn_data_in;         /**< raw trace data input function to decoder */
    fnUpdatePktMonFlags fn_update_pkt_mon;  /**< update the packet monitor / sink usage flags */

    /* Decoder instance data */
    void *decoder_handle;   /**< Instance handle for the decoder  - used by library to call the decoder CB functions */
    char *p_decoder_name;   /**< type name of the decoder - may be used in logging */
    uint8_t cs_id;          /**< Coresight ID for the instance - extracted from the config on creation. */

} ocsd_extern_dcd_inst_t;

/** function to create a decoder instance - fills in the decoder struct supplied. */
typedef ocsd_err_t (* fnCreateCustomDecoder)(const int create_flags, const void *decoder_cfg, const ocsd_extern_dcd_cb_fns *p_lib_callbacks, ocsd_extern_dcd_inst_t *p_decoder_inst);
/** Function to destroy a decoder instance - indicated by decoder handle */
typedef ocsd_err_t (* fnDestroyCustomDecoder)(const void *decoder_handle);
/** Function to extract the CoreSight Trace ID from the configuration structure */
typedef ocsd_err_t (* fnGetCSIDFromConfig)(const void *decoder_cfg, unsigned char *p_csid);
/** Function to convert a protocol specific trace packet to human readable string */
typedef ocsd_err_t (* fnPacketToString)(const void *trc_pkt, char *buffer, const int buflen);

/** set of functions and callbacks to create an extern custom decoder in the library 
    via the C API interface. This structure is registered with the library by name and 
    then decoders of the type can be created on the decode tree.    
*/
typedef struct _ocsd_extern_dcd_fact {
    /* mandatory functions */
    fnCreateCustomDecoder createDecoder;    /**< [required] Function pointer to create a decoder instance. */
    fnDestroyCustomDecoder destroyDecoder;  /**< [required] Function pointer to destroy a decoder instance. */
    fnGetCSIDFromConfig csidFromConfig;     /**< [required] Function pointer to extract the CSID from a config structure */

    /* optional functions */
    fnPacketToString pktToString;     /**< [optional] function pointer to print a trace packet in this decoder */

    ocsd_trace_protocol_t protocol_id;  /**< protocol ID assigned during registration. */
} ocsd_extern_dcd_fact_t; 


ocsd_err_t ocsd_cust_protocol_to_str(const ocsd_trace_protocol_t pkt_protocol, const void *trc_pkt, char *buffer, const int buflen);

#endif // ARM_OCSD_C_API_CUSTOM_H_INCLUDED

/* End of File ocsd_c_api_custom.h */

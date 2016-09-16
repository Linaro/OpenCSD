/*
 * \file       ocsd_c_api_custom.h
 * \brief      Reference CoreSight Trace Decoder : 
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


/* callback and registration function to connect a generic element output point */
typedef ocsd_datapath_resp_t (* fnGenElemOpCB)( const void *cb_context,
                                                const ocsd_trc_index_t index_sop, 
                                                const uint8_t trc_chan_id, 
                                                const ocsd_generic_trace_elem *elem); 
typedef ocsd_err_t (* fnRegisterGenElemOpCB)(const void *decoder_handle, const void *cb_context, const fnGenElemOpCB pFnGenElemOut);
 
/* callback and registration function to connect into the library error logging mechanism */
typedef void (* fnLogErrorCB)(const void *cb_context, const ocsd_err_severity_t filter_level, const ocsd_err_t code, const ocsd_trc_index_t idx, const uint8_t chan_id, const char *pMsg);
typedef void (* fnLogMsgCB)(const void *cb_context, const ocsd_err_severity_t filter_level, const char *msg);
typedef void (* fnRegisterErrLogCB)(const void *decoder_handle, const void *cb_context, const fnLogErrorCB pFnErrLog, const fnLogMsgCB pFnMsgLog);

/* callback and registration function to connect an ARM instruction decoder */
typedef ocsd_err_t (* fnDecodeArmInstCB)(const void *cb_context, ocsd_instr_info *instr_info);
typedef ocsd_err_t (* fnRegisterDecodeArmInstCB)(const void *decoder_handle, const void *cb_context, const fnDecodeArmInstCB pFnDecodeArmInstr);

/* callback and registration function to connect the memory accessor interface */
typedef ocsd_err_t (* fnMemAccessCB)(const void *cb_context,
                                     const ocsd_vaddr_t address, 
                                     const uint8_t cs_trace_id, 
                                     const ocsd_mem_space_acc_t mem_space, 
                                     uint32_t *num_bytes, 
                                     uint8_t *p_buffer);
typedef ocsd_err_t (* fnRegisterMemAccessCB)(const void *decoder_handle, const void *cb_context, const fnMemAccessCB pFnmemAcc);

/* callback and registration function to connect to the packet monitor interface of the packet processor */
typedef void (* fnPktMonCB)(  const void *cb_context,
                              const ocsd_datapath_op_t op,
                              const ocsd_trc_index_t index_sop,
                              const void *pkt,
                              const uint32_t size,
                              const uint8_t *p_data);
typedef ocsd_err_t (* fnRegisterPktMonCB)(const void *decoder_handle, const void *cb_context, fnPktMonCB pFnPktMon);

/* callback and registration function to connect to the packet sink interface, on the main decode 
    data path - use if decoder created as packet processor only */
typedef ocsd_datapath_resp_t (* fnPktDataSinkCB)( const ocsd_datapath_op_t op,
                                                  const ocsd_trc_index_t index_sop,
                                                  const void *p_packet_in);
typedef ocsd_err_t (* fnRegisterPktDataSinkCB)(const void *decoder_handle, const void *cb_context, fnPktDataSinkCB pFnPktData);

/** This structure is filled in by the ocsd_extern_dcd_fact_t creation function with the exception of the 
    library context value. */
typedef struct _ocsd_extern_dcd_inst {
    /* Mandatory decoder functions - library initialisation will fail without these. */
    fnTraceDataIn fn_data_in;            /**< raw trace data input function to decoder */

    /* Optional decoder functions - set to 0 if the decoder class does not require / support this functionality */
    fnRegisterGenElemOpCB       fn_reg_gen_out_cb;   /**< connect callback to get the generic element output of the decoder */
    fnRegisterErrLogCB          fn_reg_error_log_cb; /**< connect callbacks for error logging */        
    fnRegisterDecodeArmInstCB   fn_reg_instr_dcd_cb; /**< connect callback to decode an arm instruction */
    fnRegisterMemAccessCB       fn_reg_mem_acc_cb;   /**< connect callback to access trace memory images */
    fnRegisterPktMonCB          fn_reg_pkt_mon_cb;   /**< connect to the packet monitor of the packet processing stage */   
    fnRegisterPktDataSinkCB     fn_reg_pkt_sink_cb;  /**< connect to packet sink on main datapath if decoder in packe processing mode only */

    /* Decoder instance data */
    void *decoder_handle;   /**< Instance handle for the decoder */
    unsigned char cs_id;    /**< CS ID of the trace stream this decoder should be attached to */
    char *p_decoder_name;   /**< type name of the decoder - may be used in logging */

    /* opaque library context value */
    void *library_context;  /**< Context information for this decoder used by the library */
} ocsd_extern_dcd_inst_t;


/** function to create a decoder instance - fills in the decoder struct supplied. */
typedef ocsd_err_t (* fnCreateCustomDecoder)(const int create_flags, const void *decoder_cfg, ocsd_extern_dcd_inst_t *p_decoder_inst);
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

#endif // ARM_OCSD_C_API_CUSTOM_H_INCLUDED

/* End of File ocsd_c_api_custom.h */

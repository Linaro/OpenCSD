/*!
 * \file       opencsd_c_api.h
 * \brief      OpenCSD : "C" API
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
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

#ifndef ARM_OPENCSD_C_API_H_INCLUDED
#define ARM_OPENCSD_C_API_H_INCLUDED

/** @defgroup lib_c_api OpenCSD Library : Library "C" API.
    @brief  "C" API for the OpenCSD Library

    Set of "C" wrapper functions for the OpenCSD library.

    Defines API, functions and callback types.
@{*/

/* ensure C bindings */

#if defined(WIN32)  /* windows bindings */
    /** Building the C-API DLL **/
    #ifdef _OCSD_C_API_DLL_EXPORT
        #ifdef __cplusplus
            #define OCSD_C_API extern "C" __declspec(dllexport)
        #else
            #define OCSD_C_API __declspec(dllexport)
        #endif
    #else   
        /** building or using the static C-API library **/
        #if defined(_LIB) || defined(OCSD_USE_STATIC_C_API)
            #ifdef __cplusplus
                #define OCSD_C_API extern "C"
            #else
                #define OCSD_C_API
            #endif
        #else
        /** using the C-API DLL **/
            #ifdef __cplusplus
                #define OCSD_C_API extern "C" __declspec(dllimport)
            #else
                #define OCSD_C_API __declspec(dllimport)
            #endif
        #endif
    #endif
#else           /* linux bindings */
    #ifdef __cplusplus
        #define OCSD_C_API extern "C"
    #else
        #define OCSD_C_API
    #endif
#endif

#include "ocsd_c_api_types.h"

/** @name Library Version API

@{*/
/** Get Library version. Return a 32 bit version in form MMMMnnnn - MMMM = major verison, nnnn = minor version */ 
OCSD_C_API uint32_t ocsd_get_version(void);

/** Get library version string */
OCSD_C_API const char * ocsd_get_version_str(void);
/** @}*/

/*---------------------- Trace Decode Tree ----------------------------------------------------------------------------------*/

/** @name Library Decode Tree API
@{*/

/*!
 * Create a decode tree. 
 *
 * @param src_type : Type of tree - formatted input, or single source input
 * @param deformatterCfgFlags : Formatter flags - determine presence of frame syncs etc.
 *
 * @return dcd_tree_handle_t  : Handle to the decode tree. Handle value set to 0 if creation failed.
 */
OCSD_C_API dcd_tree_handle_t ocsd_create_dcd_tree(const ocsd_dcd_tree_src_t src_type, const uint32_t deformatterCfgFlags);

/*!
 * Destroy a decode tree.
 *
 * Also destroys all the associated processors and decoders for the tree.
 *
 * @param handle : Handle for decode tree to destroy.
 */
OCSD_C_API void ocsd_destroy_dcd_tree(const dcd_tree_handle_t handle);

/*!
 * Input trace data into the decoder. 
 * 
 * Large trace source buffers can be broken down into smaller fragments.
 *
 * @param handle : Handle to decode tree.
 * @param op : Datapath operation.
 * @param index : Trace buffer byte index for the start of the supplied data block.
 * @param dataBlockSize : Size of data block.
 * @param *pDataBlock : Pointer to data block.
 * @param *numBytesProcessed : Number of bytes actually processed by the decoder.
 *
 * @return ocsd_datapath_resp_t  : Datapath response code
 */
OCSD_C_API ocsd_datapath_resp_t ocsd_dt_process_data(const dcd_tree_handle_t handle,
                                            const ocsd_datapath_op_t op,
                                            const ocsd_trc_index_t index,
                                            const uint32_t dataBlockSize,
                                            const uint8_t *pDataBlock,
                                            uint32_t *numBytesProcessed);


/*---------------------- Generic Trace Element Output  --------------------------------------------------------------*/

/*!
 * Set the trace element output callback function. 
 *
 * This function will be called for each decoded generic trace element generated by 
 * any full trace decoder in the decode tree.
 *
 * A single function is used for all trace source IDs in the decode tree.
 *
 * @param handle : Handle to decode tree.
 * @param pFn : Pointer to the callback functions.
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return  ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_set_gen_elem_outfn(const dcd_tree_handle_t handle, FnTraceElemIn pFn, const void *p_context);

/*---------------------- ETMv4 Trace ----------------------------------------------------------------------------------*/
/*!
 * Create an ETMv4 instruction trace packet processor only for the supplied configuration. 
 * Must supply an output callback function which handles the etmv4 packet types, to attach to the packet processor.
 *
 * @param handle : handle a decode tree to create the packet processsor.
 * @param *etmv4_cfg : pointer to valid Etmv4 configuration structure.
 * @param pPktFn : pointer to a packet handling callback function.
 * @param p_context : opaque context pointer value used in callback function..
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_etmv4i_pkt_proc(const dcd_tree_handle_t handle, const void *etmv4_cfg, FnEtmv4IPacketDataIn pPktFn, const void *p_context);

/*!
 * Creates a packet processor + packet decoder pair for the supplied configuration structure.
 * Uses the output function set in ocsd_dt_set_gen_elem_outfn() as the output sink.
 *
 * @param handle : Handle to decode tree.
 * @param *etmv4_cfg : pointer to valid Etmv4 configuration structure.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_etmv4i_decoder(const dcd_tree_handle_t handle, const void *etmv4_cfg);

/*!
 * Attach a callback function to the packet processor monitor point defined by the CoreSight ID.
 *
 * @param handle : Handle to decode tree.
 * @param trc_chan_id : CoreSight Trace ID for packet processor 
 * @param pPktFn : Function to attach to monitor point.
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_attach_etmv4i_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnEtmv4IPktMonDataIn pPktFn, const void *p_context); 

/*---------------------- ETMv3 trace ----------------------------------------------------------------------------------*/
/*!
 * Create an ETMv3  trace packet processor only for the supplied configuration. 
 * Must supply an output callback function which handles the etmv3 packet types, to attach to the packet processor.
 *
 * @param handle : handle a decode tree to create the packet processsor.
 * @param *etmv3_cfg : pointer to valid Etmv3 configuration structure.
 * @param pPktFn : pointer to a packet handling callback function.
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_etmv3_pkt_proc(const dcd_tree_handle_t handle, const void *etmv3_cfg, FnEtmv3PacketDataIn pPktFn, const void *p_context);


/*!
 * Attach a callback function to the packet processor monitor point defined by the CoreSight ID.
 * Packet processor must exist for the trace ID and be an ETMv3 processor.
 *
 * @param handle : Handle to decode tree.
 * @param trc_chan_id : CoreSight Trace ID for packet processor 
 * @param pPktFn : Function to attach to monitor point.
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_attach_etmv3_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnEtmv3PktMonDataIn pPktFn, const void *p_context); 

/*---------------------- PTM Trace ----------------------------------------------------------------------------------*/
/*!
 * Create an PTM instruction trace packet processor only for the supplied configuration. 
 * Must supply an output callback function which handles the ptm packet types, to attach to the packet processor.
 *
 * @param handle : handle a decode tree to create the packet processsor.
 * @param *ptm_cfg : pointer to valid Ptm configuration structure.
 * @param pPktFn : pointer to a packet handling callback function.
 * @param p_context : opaque context pointer value used in callback function..
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_ptm_pkt_proc(const dcd_tree_handle_t handle, const void *ptm_cfg, FnPtmPacketDataIn pPktFn, const void *p_context);

/*!
 * Creates a packet processor + packet decoder pair for the supplied configuration structure.
 * Uses the output function set in ocsd_dt_set_gen_elem_outfn() as the output sink.
 *
 * @param handle : Handle to decode tree.
 * @param *ptm_cfg : pointer to valid Ptm configuration structure.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_ptm_decoder(const dcd_tree_handle_t handle, const void *ptm_cfg);

/*!
 * Attach a callback function to the packet processor monitor point defined by the CoreSight ID.
 *
 * @param handle : Handle to decode tree.
 * @param trc_chan_id : CoreSight Trace ID for packet processor 
 * @param pPktFn : Function to attach to monitor point.
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_attach_ptm_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnPtmPktMonDataIn pPktFn, const void *p_context); 


/*---------------------- STM Trace ----------------------------------------------------------------------------------*/
/*!
 * Create an STM  trace packet processor only for the supplied configuration. 
 * Must supply an output callback function which handles the stm packet types, to attach to the packet processor.
 *
 * @param handle : handle a decode tree to create the packet processsor.
 * @param *stm_cfg : pointer to valid Stm configuration structure.
 * @param pPktFn : pointer to a packet handling callback function.
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_stm_pkt_proc(const dcd_tree_handle_t handle, const void *stm_cfg, FnStmPacketDataIn pPktFn, const void *p_context);


/** TBD : more C API functions to be added here */    
    
/** @}*/
/*---------------------- Memory Access for traced opcodes ----------------------------------------------------------------------------------*/
/** @name Library Memory Accessor configuration on decode tree.
    @brief Configure the memory regions available for decode.
    
    Full decode requires memory regions set up to allow access to the traced
    opcodes. Add memory buffers or binary file regions to a map of regions.

@{*/

/*!
 * Add a binary file based memory range accessor to the decode tree.
 *
 * Adds the entire binary file as a memory space to be accessed
 *
 * @param handle : Handle to decode tree.
 * @param address : Start address of memory area.
 * @param mem_space : Associated memory space.
 * @param *filepath : Path to binary data file.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_add_binfile_mem_acc(const dcd_tree_handle_t handle, const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const char *filepath); 

/*!
 * Add a binary file based memory range accessor to the decode tree.
 * 
 * Add a binary file that contains multiple regions of memory with differing 
 * offsets wihtin the file.
 * 
 * A linked list of file_mem_region_t structures is supplied. Each structure contains an
 * offset into the binary file, the start address for this offset and the size of the region.
 * 
 * @param handle : Handle to decode tree.
 * @param region_list : Array of memory regions in the file.
 * @param num_regions : Size of region array
 * @param mem_space : Associated memory space.
 * @param *filepath : Path to binary data file.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_add_binfile_region_mem_acc(const dcd_tree_handle_t handle, const file_mem_region_t *region_array, const int num_regions, const ocsd_mem_space_acc_t mem_space, const char *filepath); 

/*!
 * Add a memory buffer based memory range accessor to the decode tree.
 *
 * @param handle : Handle to decode tree.
 * @param address : Start address of memory area.
 * @param mem_space : Associated memory space.
 * @param *p_mem_buffer : pointer to memory buffer.
 * @param mem_length : Size of memory buffer.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_add_buffer_mem_acc(const dcd_tree_handle_t handle, const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t *p_mem_buffer, const uint32_t mem_length); 


/*!
 * Add a memory access callback function. The decoder will call the function for opcode addresses in the 
 * address range supplied for the memory spaces covered.
 *
 * @param handle : Handle to decode tree.
 * @param st_address :  Start address of memory area covered by the callback.
 * @param en_address :  End address of the memory area covered by the callback. (inclusive)
 * @param mem_space : Memory space(s) covered by the callback.
 * @param p_cb_func : Callback function
 * @param p_context : opaque context pointer value used in callback function.
 *
 * @return OCSD_C_API ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_add_callback_mem_acc(const dcd_tree_handle_t handle, const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void *p_context); 

/*!
 * Remove a memory accessor by address and memory space.
 *
 * @param handle : Handle to decode tree.
 * @param st_address : Start address of memory accessor. 
 * @param mem_space : Memory space(s) covered by the accessor.
 *
 * @return OCSD_C_API ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_remove_mem_acc(const dcd_tree_handle_t handle, const ocsd_vaddr_t st_address, const ocsd_mem_space_acc_t mem_space);

/*
 *  Print the mapped memory accessor ranges to the configured logger.
 *
 * @param handle : Handle to decode tree.
 */
OCSD_C_API void ocsd_tl_log_mapped_mem_ranges(const dcd_tree_handle_t handle);

/** @}*/  

/** @name Library Default Error Log Object API
    @brief Configure the default error logging object in the library.

    Objects created by the decode trees will use this error logger. Configure for 
    desired error severity, and to enable print or logfile output.

@{*/

/*---------------------- Library Logging and debug ----------------------------------------------------------------------------------*/
/*!
 * Initialise the library error logger. 
 *
 * Choose severity of errors logger, and if the errors will be logged to screen and / or logfile.
 *
 * @param verbosity : Severity of errors that will be logged.
 * @param create_output_logger : Set to none-zero to create an output printer.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful. 
 */
OCSD_C_API ocsd_err_t ocsd_def_errlog_init(const ocsd_err_severity_t verbosity, const int create_output_logger);

/*!
 * Configure the output logger. Choose STDOUT, STDERR and/or log to file.
 * Optionally provide a log file name.
 *
 * @param output_flags : OR combination of required  C_API_MSGLOGOUT_FLG_* flags.
 * @param *log_file_name : optional filename if logging to file. Set to NULL if not needed.
 *
 * @return OCSD_C_API ocsd_err_t  : Library error code -  RCDTL_OK if successful. 
 */
OCSD_C_API ocsd_err_t ocsd_def_errlog_config_output(const int output_flags, const char *log_file_name);

/*!
 * Print a message via the library output printer - if enabled.
 *
 * @param *msg : Message to output.
 *
 */
OCSD_C_API void ocsd_def_errlog_msgout(const char *msg);


/** @}*/

/** @name Packet to string interface

@{*/

/*!
 * Take a packet structure and render a string representation of the packet data.
 * 
 * Returns a '0' terminated string of (buffer_size - 1) length or less.
 *
 * @param pkt_protocol : Packet protocol type - used to interpret the packet pointer
 * @param *p_pkt : pointer to a valid packet structure of protocol type. cast to void *.
 * @param *buffer : character buffer for string.
 * @param buffer_size : size of character buffer.
 *
 * @return  ocsd_err_t  : Library error code -  RCDTL_OK if successful. 
 */
OCSD_C_API ocsd_err_t ocsd_pkt_str(const ocsd_trace_protocol_t pkt_protocol, const void *p_pkt, char *buffer, const int buffer_size);

/*!
 * Get a string representation of the generic trace element.
 *
 * @param *p_pkt : pointer to valid generic element structure.
 * @param *buffer : character buffer for string.
 * @param buffer_size : size of character buffer.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful. 
 */
OCSD_C_API ocsd_err_t ocsd_gen_elem_str(const ocsd_generic_trace_elem *p_pkt, char *buffer, const int buffer_size);

/** @}*/

/** @}*/

#endif // ARM_OPENCSD_C_API_H_INCLUDED


/* End of File opencsd_c_api.h */

/*!
 * \file       ocsd_c_api_deprc_fn.h
 * \brief      OpenCSD : Deprecated C-API functions
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

#ifndef ARM_OCSD_C_API_DEPRC_FN_H_INCLUDED
#define ARM_OCSD_C_API_DEPRC_FN_H_INCLUDED

/** @name Deprecated C-API functions
@{*/

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
 * Creates a packet processor + packet decoder pair for the supplied configuration structure.
 * Uses the output function set in ocsd_dt_set_gen_elem_outfn() as the output sink.
 *
 * @param handle : Handle to decode tree.
 * @param *etmv4_cfg : pointer to valid Etmv4 configuration structure.
 *
 * @return ocsd_err_t  : Library error code -  RCDTL_OK if successful.
 */
OCSD_C_API ocsd_err_t ocsd_dt_create_etmv3_decoder(const dcd_tree_handle_t handle, const void *etmv3_cfg);



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

/** @}*/

#endif // ARM_OCSD_C_API_DEPRC_FN_H_INCLUDED

/* End of File ocsd_c_api_deprc_fn.h */

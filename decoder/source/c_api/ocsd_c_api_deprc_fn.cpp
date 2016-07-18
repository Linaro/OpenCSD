/*
 * \file       ocsd_c_api_deprc_fn.cpp
 * \brief      OpenCSD : Deprecated C-API functions.
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

/* pull in the C++ decode library */
#include "opencsd.h"

/* C-API and wrapper objects */
#define OPENCSD_INC_DEPRECATED_API
#include "c_api/opencsd_c_api.h"
#include "ocsd_c_api_obj.h"

#ifdef OPENCSD_INC_DEPRECATED_API

OCSD_C_API ocsd_err_t ocsd_dt_create_etmv4i_pkt_proc(const dcd_tree_handle_t handle, const void *etmv4_cfg, FnEtmv4IPacketDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    uint8_t CSID = 0;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_ETMV4I,OCSD_CREATE_FLG_PACKET_PROC,etmv4_cfg,&CSID);
        if(err == OCSD_OK)
        {
            err = ocsd_dt_attach_packet_callback(handle,CSID, OCSD_C_API_CB_PKT_SINK,pPktFn,p_context);
            if(err != OCSD_OK)
                ocsd_dt_remove_decoder(handle,CSID);
        }        
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_create_etmv4i_decoder(const dcd_tree_handle_t handle, const void *etmv4_cfg)
{
    ocsd_err_t err = OCSD_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        uint8_t CSID = 0;
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_ETMV4I,OCSD_CREATE_FLG_FULL_DECODER,etmv4_cfg,&CSID);
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_attach_etmv4i_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnEtmv4IPktMonDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_attach_packet_callback(handle,trc_chan_id,OCSD_C_API_CB_PKT_MON,pPktFn,p_context);
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_create_etmv3_pkt_proc(const dcd_tree_handle_t handle, const void *etmv3_cfg, FnEtmv3PacketDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    uint8_t CSID = 0;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_ETMV3,OCSD_CREATE_FLG_PACKET_PROC,etmv3_cfg,&CSID);
        if(err == OCSD_OK)
        {
            err = ocsd_dt_attach_packet_callback(handle,CSID, OCSD_C_API_CB_PKT_SINK,pPktFn,p_context);
            if(err != OCSD_OK)
                ocsd_dt_remove_decoder(handle,CSID);
        }        
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_create_etmv3_decoder(const dcd_tree_handle_t handle, const void *etmv3_cfg)
{
    ocsd_err_t err = OCSD_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        uint8_t CSID = 0;
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_ETMV3,OCSD_CREATE_FLG_FULL_DECODER,etmv3_cfg,&CSID);
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_attach_etmv3_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnEtmv3PktMonDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_attach_packet_callback(handle,trc_chan_id,OCSD_C_API_CB_PKT_MON,pPktFn,p_context);
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}


OCSD_C_API ocsd_err_t ocsd_dt_create_ptm_pkt_proc(const dcd_tree_handle_t handle, const void *ptm_cfg, FnPtmPacketDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    uint8_t CSID = 0;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_PTM,OCSD_CREATE_FLG_PACKET_PROC,ptm_cfg,&CSID);
        if(err == OCSD_OK)
        {
            err = ocsd_dt_attach_packet_callback(handle,CSID, OCSD_C_API_CB_PKT_SINK,pPktFn,p_context);
            if(err != OCSD_OK)
                ocsd_dt_remove_decoder(handle,CSID);
        }        
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;

}

OCSD_C_API ocsd_err_t ocsd_dt_create_ptm_decoder(const dcd_tree_handle_t handle, const void *ptm_cfg)
{
    ocsd_err_t err = OCSD_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        uint8_t CSID = 0;
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_PTM,OCSD_CREATE_FLG_FULL_DECODER,ptm_cfg,&CSID);
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_attach_ptm_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnPtmPktMonDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_attach_packet_callback(handle,trc_chan_id,OCSD_C_API_CB_PKT_MON,pPktFn,p_context);
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

OCSD_C_API ocsd_err_t ocsd_dt_create_stm_pkt_proc(const dcd_tree_handle_t handle, const void *stm_cfg, FnStmPacketDataIn pPktFn, const void *p_context)
{
    ocsd_err_t err = OCSD_OK;
    uint8_t CSID = 0;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        err = ocsd_dt_create_decoder(handle,OCSD_BUILTIN_DCD_STM,OCSD_CREATE_FLG_PACKET_PROC,stm_cfg,&CSID);
        if(err == OCSD_OK)
        {
            err = ocsd_dt_attach_packet_callback(handle,CSID, OCSD_C_API_CB_PKT_SINK,pPktFn,p_context);
            if(err != OCSD_OK)
                ocsd_dt_remove_decoder(handle,CSID);
        }        
    }
    else
        err = OCSD_ERR_INVALID_PARAM_VAL;
    return err;
}

#endif

/* End of File ocsd_c_api_deprc_fn.cpp */

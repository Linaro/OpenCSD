/*
 * \file       trc_pkt_prco_ptm_impl.cpp
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

#include "trc_pkt_proc_ptm_impl.h"

PtmPktProcImpl::PtmPktProcImpl() :
    m_isInit(false),
    m_interface(0)
{
}

PtmPktProcImpl::~PtmPktProcImpl()
{
}

rctdl_err_t PtmPktProcImpl::Configure(const PtmConfig *p_config)
{
    rctdl_err_t err = RCTDL_OK;
    if(p_config != 0)
        m_config = *p_config;
    else
    {
        err = RCTDL_ERR_INVALID_PARAM_VAL;
        if(m_isInit)
            m_interface->LogError(rctdlError(RCTDL_ERR_SEV_ERROR,err));
    }
    return err;
}

rctdl_datapath_resp_t PtmPktProcImpl::processData(const rctdl_trc_index_t index,
                                                    const uint32_t dataBlockSize,
                                                    const uint8_t *pDataBlock,
                                                    uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t PtmPktProcImpl::onEOT()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t PtmPktProcImpl::onReset()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t PtmPktProcImpl::onFlush()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

 void PtmPktProcImpl::Initialise(TrcPktProcPtm *p_interface)
 {
     if(p_interface)
     {
         m_interface = p_interface;
         m_isInit = true;
     }
 }




/* End of File trc_pkt_prco_ptm_impl.cpp */

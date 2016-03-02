/*!
 * \file       trc_pkt_decode_etmv3.cpp
 * \brief      Reference CoreSight Trace Decoder : ETMv3 trace packet decode.
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

#include "etmv3/trc_pkt_decode_etmv3.h"

#define DCD_NAME "DCD_ETMV3"

TrcPktDecodeEtmV3::TrcPktDecodeEtmV3() : 
    TrcPktDecodeBase(DCD_NAME)
{
    initDecoder();
}

TrcPktDecodeEtmV3::TrcPktDecodeEtmV3(int instIDNum) :
    TrcPktDecodeBase(DCD_NAME, instIDNum)
{
    initDecoder();
}

TrcPktDecodeEtmV3::~TrcPktDecodeEtmV3()
{
}


/* implementation packet decoding interface */
rctdl_datapath_resp_t TrcPktDecodeEtmV3::processPacket()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV3::onEOT()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    
    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV3::onReset()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    resetDecoder();
    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV3::onFlush()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_err_t TrcPktDecodeEtmV3::onProtocolConfig()
{
    rctdl_err_t err = RCTDL_OK;
    
    // set some static config elements
    m_CSID = m_config->getTraceID();

    // check config compatible with current decoder support level.
    // at present no data trace;
    if(m_config->GetTraceMode() != EtmV3Config::TM_INSTR_ONLY)
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv3 trace decoder : data trace decode not yet supported"));
    }
    return err;
}

/* local decode methods */
void TrcPktDecodeEtmV3::initDecoder()
{


    resetDecoder();
}

void TrcPktDecodeEtmV3::resetDecoder()
{
    m_curr_state = NO_SYNC;

}



/* End of File trc_pkt_decode_etmv3.cpp */

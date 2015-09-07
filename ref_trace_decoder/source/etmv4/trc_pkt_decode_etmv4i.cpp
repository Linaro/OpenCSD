/*
 * \file       trc_pkt_decode_etmv4i.cpp
 * \brief      Reference CoreSight Trace Decoder : ETMv4 decoder
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

#include "etmv4/trc_pkt_decode_etmv4i.h"

#include "trc_etmv4_stack_elem.h"
#include "trc_gen_elem.h"


#define DCD_NAME "DCD_ETMV4"

TrcPktDecodeEtmV4I::TrcPktDecodeEtmV4I()
    : TrcPktDecodeBase(DCD_NAME)
{
    initDecoder();
}

TrcPktDecodeEtmV4I::TrcPktDecodeEtmV4I(int instIDNum)
    : TrcPktDecodeBase(DCD_NAME,instIDNum)
{
    initDecoder();
}

TrcPktDecodeEtmV4I::~TrcPktDecodeEtmV4I()   
{
    delete m_pAddrRegs;
}

/*********************** implementation packet decoding interface */

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::processPacket()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    bool bPktDone = false;

    while(!bPktDone)
    {
        switch (m_curr_state)
        {
        case NO_SYNC:
            {
                RctdlTraceElement elem(RCTDL_GEN_TRC_ELEM_NO_SYNC);
                resp = outputTraceElement(elem);
                m_curr_state = WAIT_SYNC;
                if(!RCTDL_DATA_RESP_IS_CONT(resp))
                    bPktDone = true;
            }
            break;

        case WAIT_SYNC:
            if(m_curr_packet_in->getType() == ETM4_PKT_I_ASYNC)
                m_curr_state = WAIT_TINFO;
            bPktDone = true;
            break;

        case WAIT_TINFO:
            m_need_ctxt = true;
            m_need_addr = true;
            if(m_curr_packet_in->getType() == ETM4_PKT_I_TRACE_INFO)
            {
                doTraceInfoPacket();
                m_curr_state = DECODE_PKTS;
                bPktDone = true;
            }
            break;

        case DECODE_PKTS:
            resp = decodePacket(bPktDone);  // this may change the state to commit elem;
            break;

        case COMMIT_ELEM:
            resp = commitElements(bPktDone);
            break;

        }
    }
    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::onEOT()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::onReset()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    resetDecoder();
    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::onFlush()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    if((m_curr_state == NO_SYNC) || (m_curr_state == COMMIT_ELEM))
        resp = processPacket(); // continue ongoing operation
    return resp;
}

rctdl_err_t TrcPktDecodeEtmV4I::onProtocolConfig()
{
    rctdl_err_t err = RCTDL_OK;

    // set some static config elements
    m_CSID = m_config->getTraceID();
    m_max_spec_depth = m_config->MaxSpecDepth();
    m_p0_key_max = m_config->P0_Key_Max();
    m_cond_key_max_incr = m_config->CondKeyMaxIncr();

    // check config compatible with current decoder support level.
    // at present no data trace, no spec depth, no return stack, no QE
    // Remove these checks as support is added.
    if(m_max_spec_depth != 0)
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv4 instruction decode : None-zero speculation depth not supported"));
    }
    else if(m_config->enabledDataTrace())
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv4 instruction decode : Data trace elements not supported"));
    }
    else if(m_config->enabledLSP0Trace())
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv4 instruction decode : LSP0 elements not supported."));
    }
    else if(m_config->enabledCondITrace() != EtmV4Config::COND_TR_DIS)
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv4 instruction decode : Trace on conditional non-branch elements not supported."));
    }
    else if(m_config->enabledRetStack())
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv4 instruction decode : Trace using return stack not supported."));
    }
    else if(m_config->enabledQE())
    {
        err = RCTDL_ERR_HW_CFG_UNSUPP;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_HW_CFG_UNSUPP,"ETMv4 instruction decode : Trace using Q elements not supported."));
    }
    return err;
}


/************* local decode methods */
void TrcPktDecodeEtmV4I::initDecoder()
{
     /* init elements that get set by config */
     m_max_spec_depth = 0;
     m_p0_key_max = 0;
     m_CSID = 0;
     m_cond_key_max_incr = 0;

     // set up the broadcast address stack
     m_pAddrRegs = new (std::nothrow) AddrValStack();

     // reset decoder state to unsynced
     resetDecoder();
}

void TrcPktDecodeEtmV4I::resetDecoder()
{
    m_curr_state = NO_SYNC;
    m_timestamp = 0;
    m_context_id = 0;              
    m_vmid_id = 0;                 
    m_is_secure = true;
    m_is_64bit = false;
    m_cc_threshold = 0;
    m_curr_spec_depth = 0;
    m_p0_key = 0;
    m_cond_c_key = 0;
    m_cond_r_key = 0;
    m_need_ctxt = true;
    m_need_addr = true;

}


// this function can output an immediate generic element if this covers the complete packet decode, or stack P0 and other elements for later 
// processing on commit or cancel.
rctdl_datapath_resp_t TrcPktDecodeEtmV4I::decodePacket(bool &Complete)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    Complete = true;

    switch(m_curr_packet_in->getType())
    {
    case ETM4_PKT_I_TRACE_INFO:
        // skip subsequent TInfo packets.
        break;

    case ETM4_PKT_I_ATOM_F1:
    case ETM4_PKT_I_ATOM_F2:
    case ETM4_PKT_I_ATOM_F3:
    case ETM4_PKT_I_ATOM_F4:
    case ETM4_PKT_I_ATOM_F5:
    case ETM4_PKT_I_ATOM_F6:
        {
            TrcStackElemAtom *pElem = new (std::nothrow) TrcStackElemAtom(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setAtom(m_curr_packet_in->getAtom());
                m_P0_stack.push_front(pElem);
                m_curr_spec_depth += m_curr_packet_in->getAtom().num;
            }
        }
        break;





    }

    // auto commit anything above max spec depth 
    // (this will auto commit anything if spec depth not supported!)
    if(m_curr_spec_depth > m_max_spec_depth)
    {
        m_P0_commit = m_curr_spec_depth - m_max_spec_depth;
        m_curr_state = COMMIT_ELEM;
        Complete = false;   // force the processing of the commit elements.
    }
    return resp;
}

void TrcPktDecodeEtmV4I::doTraceInfoPacket()
{
    m_trace_info = m_curr_packet_in->getTraceInfo();
    m_cc_threshold = m_curr_packet_in->getCCThreshold();
    m_p0_key = m_curr_packet_in->getP0Key();
    m_curr_spec_depth = m_curr_packet_in->getCurrSpecDepth();
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::commitElements(bool &Complete)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

/* End of File trc_pkt_decode_etmv4i.cpp */

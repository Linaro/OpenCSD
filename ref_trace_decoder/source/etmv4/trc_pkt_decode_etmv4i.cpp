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
    m_except_pending_addr_ctxt = false;
}


// this function can output an immediate generic element if this covers the complete packet decode, 
// or stack P0 and other elements for later processing on commit or cancel.
rctdl_datapath_resp_t TrcPktDecodeEtmV4I::decodePacket(bool &Complete)
{
    bool bAllocErr = false;
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    Complete = true;
    

    switch(m_curr_packet_in->getType())
    {
    case ETM4_PKT_I_TRACE_INFO:
        // skip subsequent TInfo packets.
        break;

    case ETM4_PKT_I_TRACE_ON:
        {
            TrcStackElemParam *pElem = 
                new (std::nothrow) TrcStackElemParam( P0_TRC_ON, false, m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
                m_P0_stack.push_front(pElem);
            else
                bAllocErr = true;
        }
        break;

    case ETM4_PKT_I_OVERFLOW:
        {
            TrcStackElemParam *pElem = 
                new (std::nothrow) TrcStackElemParam( P0_OVERFLOW, false, m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
                m_P0_stack.push_front(pElem);
            else
                bAllocErr = true;
        }
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
            else
                bAllocErr = true;

        }
        break;

    case ETM4_PKT_I_CTXT:
        if(m_except_pending_addr_ctxt)
        {
            TrcStackElemExcept *pElem = dynamic_cast<TrcStackElemExcept *>(m_P0_stack[0]);
            if(pElem)
            {
                m_except_pending_addr_ctxt = false;
                m_curr_spec_depth++;
                pElem->setContext(m_curr_packet_in->getContext());
            }
        }
        else
        {
            TrcStackElemCtxt *pElem = new (std::nothrow) TrcStackElemCtxt(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setContext(m_curr_packet_in->getContext());
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;

        }
    case ETM4_PKT_I_ADDR_MATCH:
        if(m_except_pending_addr_ctxt)
        {
            TrcStackElemExcept *pElem = dynamic_cast<TrcStackElemExcept *>(m_P0_stack[0]);
            if(pElem)
            {
                m_except_pending_addr_ctxt = false;
                m_curr_spec_depth++;
                pElem->setAddr(m_pAddrRegs->get(m_curr_packet_in->getAddrMatch()));
            }
        }
        else
        {
            TrcStackElemAddr *pElem = new (std::nothrow) TrcStackElemAddr(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setAddr(m_pAddrRegs->get(m_curr_packet_in->getAddrMatch()));
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;
        }
        break;

    case ETM4_PKT_I_ADDR_CTXT_L_32IS0:
    case ETM4_PKT_I_ADDR_CTXT_L_32IS1:       
    case ETM4_PKT_I_ADDR_CTXT_L_64IS0:
    case ETM4_PKT_I_ADDR_CTXT_L_64IS1:
        if(m_except_pending_addr_ctxt)
        {
            TrcStackElemExcept *pElem = dynamic_cast<TrcStackElemExcept *>(m_P0_stack[0]);
            if(pElem)
            {
                pElem->setContext(m_curr_packet_in->getContext());
            }
        }
        else
        {
            TrcStackElemCtxt *pElem = new (std::nothrow) TrcStackElemCtxt(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setContext(m_curr_packet_in->getContext());
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;

        }
    case ETM4_PKT_I_ADDR_L_32IS0:
    case ETM4_PKT_I_ADDR_L_32IS1:         
    case ETM4_PKT_I_ADDR_L_64IS0:
    case ETM4_PKT_I_ADDR_L_64IS1:   
    case ETM4_PKT_I_ADDR_S_IS0:
    case ETM4_PKT_I_ADDR_S_IS1:
        if(m_except_pending_addr_ctxt)
        {
            TrcStackElemExcept *pElem = dynamic_cast<TrcStackElemExcept *>(m_P0_stack[0]);
            if(pElem)
            {
                m_except_pending_addr_ctxt = false;
                m_curr_spec_depth++;
                etmv4_addr_val_t addr;
                addr.val = m_curr_packet_in->getAddrVal();
                addr.isa = m_curr_packet_in->getAddrIS();
                pElem->setAddr(addr);
            }
        }
        else
        {
            TrcStackElemAddr *pElem = new (std::nothrow) TrcStackElemAddr(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                etmv4_addr_val_t addr;
                addr.val = m_curr_packet_in->getAddrVal();
                addr.isa = m_curr_packet_in->getAddrIS();
                pElem->setAddr(addr);
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;

        }
        break;

    // Exceptions
    case ETM4_PKT_I_EXCEPT:
         {
            TrcStackElemCtxt *pElem = new (std::nothrow) TrcStackElemCtxt(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                m_P0_stack.push_front(pElem);
                m_except_pending_addr_ctxt = true;  // wait for following packets
            }
            else
                bAllocErr = true;

         }
         break;

    case ETM4_PKT_I_EXCEPT_RTN:
        {
            // P0 element if V7M profile.
            bool bV7MProfile = (m_config->archVersion() == ARCH_V7) && (m_config->coreProfile() == profile_CortexM);
            TrcStackElemParam *pElem = 
                new (std::nothrow) TrcStackElemParam(P0_EXCEP_RET, bV7MProfile, m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
                m_P0_stack.push_front(pElem);
            else
                bAllocErr = true;

        }
        break;

    // event trace
    case ETM4_PKT_I_EVENT:
        {
            TrcStackElemParam *pElem = 
                new (std::nothrow) TrcStackElemParam( P0_EVENT, false, m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setParam(m_curr_packet_in->event_val,0);
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;

        }
        break;

    /* cycle count packets */
    case ETM4_PKT_I_CCNT_F1:
    case ETM4_PKT_I_CCNT_F2:
    case ETM4_PKT_I_CCNT_F3:
        {
            TrcStackElemParam *pElem = 
                new (std::nothrow) TrcStackElemParam( P0_CC, false, m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setParam(m_curr_packet_in->getCC(),0);
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;

        }
        break;

    // timestamp
    case ETM4_PKT_I_TIMESTAMP:
        {
            bool bTSwithCC = m_config->enabledCCI();
            TrcStackElemParam *pElem = 
                new (std::nothrow) TrcStackElemParam( bTSwithCC ? P0_TS_CC : P0_TS, false, m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                uint64_t ts = m_curr_packet_in->getTS();
                pElem->setParam((uint32_t)(ts & 0xFFFFFFFF),0);
                pElem->setParam((uint32_t)((ts>>32) & 0xFFFFFFFF),1);
                if(bTSwithCC)
                    pElem->setParam(m_curr_packet_in->getCC(),2);
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;

        }
        break;

    /*** presently unsupported packets ***/
    /* conditional instruction tracing */
    case ETM4_PKT_I_COND_FLUSH:
    case ETM4_PKT_I_COND_I_F1:
    case ETM4_PKT_I_COND_I_F2:
    case ETM4_PKT_I_COND_I_F3:
    case ETM4_PKT_I_COND_RES_F1:
    case ETM4_PKT_I_COND_RES_F2:
    case ETM4_PKT_I_COND_RES_F3:
    case ETM4_PKT_I_COND_RES_F4:
    // speculation 
    case ETM4_PKT_I_CANCEL_F1:
    case ETM4_PKT_I_CANCEL_F2:
    case ETM4_PKT_I_CANCEL_F3:
    case ETM4_PKT_I_COMMIT:
    case ETM4_PKT_I_MISPREDICT:
    case ETM4_PKT_I_DISCARD:
    // data synchronisation markers
    case ETM4_PKT_I_NUM_DS_MKR:
    case ETM4_PKT_I_UNNUM_DS_MKR:
    /* Q packets */
    case ETM4_PKT_I_Q:
        resp = RCDTL_RESP_FATAL_INVALID_DATA;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_BAD_DECODE_PKT,"Unsupported packet type."));
        break;

    default:
        // any other packet - bad packet error
        resp = RCDTL_RESP_FATAL_INVALID_DATA;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_BAD_DECODE_PKT,"Unknown packet type."));
        break;

    }

    if(bAllocErr)
    {
        resp = RCTDL_RESP_FATAL_SYS_ERR;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_MEM,"Memory allocation error."));       
    }
    else if(m_curr_spec_depth > m_max_spec_depth)
    {
        // auto commit anything above max spec depth 
        // (this will auto commit anything if spec depth not supported!)
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

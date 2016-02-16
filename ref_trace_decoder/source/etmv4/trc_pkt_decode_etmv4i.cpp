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

static const uint32_t ETMV4_SUPPORTED_DECODE_OP_FLAGS = RCTDL_OPFLG_PKTDEC_COMMON;

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
            // output the initial not synced packet to the sink
            m_output_elem.setType(RCTDL_GEN_TRC_ELEM_NO_SYNC);
            resp = outputTraceElement(m_output_elem);
            m_curr_state = WAIT_SYNC;
            // fall through to check if the current packet is the async we are waiting for.
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
            }
            bPktDone = true;
            break;

        case DECODE_PKTS:
            resp = decodePacket(bPktDone);  // this may change the state to commit elem;
            break;

        case COMMIT_ELEM:
            resp = commitElements(bPktDone); // this will change the state to DECODE_PKTS once all elem committed.
            break;

        }
    }
    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::onEOT()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    m_output_elem.setType(RCTDL_GEN_TRC_ELEM_EO_TRACE);
    resp = outputTraceElement(m_output_elem);
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
    if(m_curr_state == COMMIT_ELEM)
        resp = processPacket(); // continue ongoing output operation
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

    // set up static trace instruction decode elements
    m_instr_info.dsb_dmb_waypoints = 0;
    m_instr_info.pe_type.arch = m_config->archVersion();
    m_instr_info.pe_type.profile = m_config->coreProfile();

    m_IASize64 = (m_config->iaSizeMax() == 64);

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
    // set the operational modes supported.
    m_supported_op_flags = ETMV4_SUPPORTED_DECODE_OP_FLAGS;

    /* init elements that get set by config */
    m_max_spec_depth = 0;
    m_p0_key_max = 0;
    m_CSID = 0;
    m_cond_key_max_incr = 0;
    m_IASize64 = false;

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
    m_except_pending_addr = false;
    m_mem_nacc_pending = false;
    m_prev_overflow = false;

    etmv4_addr_val_t addr;
    addr.isa = 0;
    addr.val = 0;
    
    m_pAddrRegs->push(addr);    // preload first value with 0x0
    m_P0_stack.clear();
    m_output_elem.init();
}

// this function can output an immediate generic element if this covers the complete packet decode, 
// or stack P0 and other elements for later processing on commit or cancel.
rctdl_datapath_resp_t TrcPktDecodeEtmV4I::decodePacket(bool &Complete)
{
    bool bAllocErr = false;
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    Complete = true;
    bool is_addr = false;
    bool is_except = false;
    bool is_64L = false;
    
    switch(m_curr_packet_in->getType())
    {
    case ETM4_PKT_I_ASYNC: // nothing to do with this packet.
        break;

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
        break;

    case ETM4_PKT_I_ADDR_MATCH:
        {
        TrcStackElemAddr *pElem = new (std::nothrow) TrcStackElemAddr(m_curr_packet_in->getType(), m_index_curr_pkt);
        if(pElem)
        {
            pElem->setAddrMatch(m_curr_packet_in->getAddrMatch());  // must wait till speculation is resolved before we know the rigth address / index match
            m_P0_stack.push_front(pElem);
        }
        else
            bAllocErr = true;
        is_addr = true;
        }
        break;

    case ETM4_PKT_I_ADDR_CTXT_L_64IS0:
    case ETM4_PKT_I_ADDR_CTXT_L_64IS1:
        is_64L = true;
    case ETM4_PKT_I_ADDR_CTXT_L_32IS0:
    case ETM4_PKT_I_ADDR_CTXT_L_32IS1:    
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
        if((m_curr_packet_in->getType() == ETM4_PKT_I_ADDR_L_64IS0) ||
           (m_curr_packet_in->getType() == ETM4_PKT_I_ADDR_L_64IS1))
           is_64L = true;
    case ETM4_PKT_I_ADDR_S_IS0:
    case ETM4_PKT_I_ADDR_S_IS1:
        {
            TrcStackElemAddr *pElem = new (std::nothrow) TrcStackElemAddr(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                etmv4_addr_val_t addr;

                addr.val = m_curr_packet_in->getAddrVal();
                addr.isa = m_curr_packet_in->getAddrIS();
                pElem->setAddr(addr,is_64L);
                m_P0_stack.push_front(pElem);
            }
            else
                bAllocErr = true;
            is_addr = true;
        }
        break;

    // Exceptions
    case ETM4_PKT_I_EXCEPT:
         {
            TrcStackElemExcept *pElem = new (std::nothrow) TrcStackElemExcept(m_curr_packet_in->getType(), m_index_curr_pkt);
            if(pElem)
            {
                pElem->setPrevSame(m_curr_packet_in->exception_info.addr_interp == 0x1);
                m_P0_stack.push_front(pElem);
                m_except_pending_addr = true;  // wait for following packets before marking for commit.
                is_except = true;
                pElem->setExcepNum(m_curr_packet_in->exception_info.exceptionType);
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

    case ETM4_PKT_I_BAD_SEQUENCE:
        resp = handleBadPacket("Bad byte sequence in packet.");
        break;

    case ETM4_PKT_I_BAD_TRACEMODE:
        resp = handleBadPacket("Invalid packet type for trace mode.");
        break;

    case ETM4_PKT_I_RESERVED:
        resp = handleBadPacket("Reserved packet header");
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

    // we need to wait for following address after exception 
    // - work out if we have seen enough here...
    if(m_except_pending_addr && !is_except)
    {
        m_except_pending_addr = false;  //next packet has to be an address
        // exception packet sequence complete
        if(is_addr)
        {
            m_curr_spec_depth++;   // exceptions are P0 elements so up the spec depth to commit if needed.
        }
        else
        {
            // TBD: error out here on none address 
        }
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

/*
 * Walks through the element stack, processing from oldest element to the newest, 
   according to the number of P0 elements that need committing.
 */
rctdl_datapath_resp_t TrcPktDecodeEtmV4I::commitElements(bool &Complete)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    bool bPause = false;    // pause commit operation 
    bool bPopElem = true;       // do we remove the element from the stack (multi atom elements may need to stay!)
    int num_commit_req = m_P0_commit;

    Complete = true; // assume we exit due to completion of commit operation

    TrcStackElem *pElem = 0;    // stacked element pointer

    while(m_P0_commit && !bPause)
    {
        if(m_P0_stack.size() > 0)
        {
            pElem = m_P0_stack.back();  // get oldest element
            
            switch(pElem->getP0Type())
            {
            // indicates a trace restart - beginning of trace or discontinuiuty
            case P0_TRC_ON:
                m_output_elem.setType(RCTDL_GEN_TRC_ELEM_TRACE_ON);
                m_output_elem.trace_on_reason = m_prev_overflow ? TRACE_ON_OVERFLOW : TRACE_ON_NORMAL;
                m_prev_overflow = false;
                resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                break;

            case P0_ADDR:
                {
                TrcStackElemAddr *pAddrElem = dynamic_cast<TrcStackElemAddr *>(pElem);
                if(pAddrElem)
                {
                    int match_idx = 0;
                    if(pAddrElem->isAddrMatch(match_idx))
                    {
                        SetInstrInfoInAddrISA(m_pAddrRegs->get(match_idx).val,m_pAddrRegs->get(match_idx).isa);
                    }
                    else
                    {
                        // if 64 bit value, or IAsize is 32 bit only...
                        if(!m_IASize64 || pAddrElem->is64bit())
                        {
                            // use value immediately
                            SetInstrInfoInAddrISA(pAddrElem->getAddr().val,pAddrElem->getAddr().isa);
                            m_pAddrRegs->push(pAddrElem->getAddr());
                        }
                        else
                        {
                            // otherwise 32 bit values must add in the top 32 from the stack
                            etmv4_addr_val_t new_addr = m_pAddrRegs->get(0);
                            new_addr.val &= ~((rctdl_vaddr_t)0xFFFFFFFF);
                            new_addr.val |= (pAddrElem->getAddr().val & 0xFFFFFFFF);
                            new_addr.isa = pAddrElem->getAddr().isa;
                            SetInstrInfoInAddrISA(new_addr.val,new_addr.isa);
                            m_pAddrRegs->push(new_addr);
                        }                                                                                                
                    }
                    m_need_addr = false;
                }
                }
                break;

            case P0_CTXT:
                {
                TrcStackElemCtxt *pCtxtElem = dynamic_cast<TrcStackElemCtxt *>(pElem);
                if(pCtxtElem)
                {
                    etmv4_context_t ctxt = pCtxtElem->getContext();
                    // check this is an updated context
                    if(ctxt.updated)
                    {
                        updateContext(pCtxtElem);

                        m_output_elem.setType(RCTDL_GEN_TRC_ELEM_PE_CONTEXT);
                        resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                    }
                }
                }
                break;

            case P0_EVENT:
                {
                TrcStackElemParam *pParamElem = dynamic_cast<TrcStackElemParam *>(pElem);
                if(pParamElem)
                {
                    m_output_elem.setType(RCTDL_GEN_TRC_ELEM_EVENT);
                    m_output_elem.gen_value = pParamElem->getParam(0);
                    resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                }
                }
                break;

            case P0_TS:
                {
                TrcStackElemParam *pParamElem = dynamic_cast<TrcStackElemParam *>(pElem);
                if(pParamElem)
                {
                    m_output_elem.setType(RCTDL_GEN_TRC_ELEM_TIMESTAMP);
                    m_output_elem.timestamp = (uint64_t)(pParamElem->getParam(0)) | (((uint64_t)pParamElem->getParam(1)) << 32);
                    resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                }
                }
                break;

            case P0_CC:
                {
                TrcStackElemParam *pParamElem = dynamic_cast<TrcStackElemParam *>(pElem);
                if(pParamElem)
                {
                    m_output_elem.setType(RCTDL_GEN_TRC_ELEM_CYCLE_COUNT);
                    m_output_elem.cycle_count = pParamElem->getParam(0);
                    resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                }
                }
                break;

            case P0_TS_CC:
                {
                TrcStackElemParam *pParamElem = dynamic_cast<TrcStackElemParam *>(pElem);
                if(pParamElem)
                {
                    m_output_elem.setType(RCTDL_GEN_TRC_ELEM_TIMESTAMP);
                    m_output_elem.timestamp = (uint64_t)(pParamElem->getParam(0)) | (((uint64_t)pParamElem->getParam(1)) << 32);
                    m_output_elem.setCycleCount(pParamElem->getParam(2));
                    resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                }
                }
                break;

            case P0_OVERFLOW:
                m_prev_overflow = true;
                break;

            case P0_ATOM:
                {
                TrcStackElemAtom *pAtomElem = dynamic_cast<TrcStackElemAtom *>(pElem);
                if(pAtomElem)
                {
                    bool bContProcess = true;
                    while(!pAtomElem->isEmpty() && m_P0_commit && bContProcess)
                    {
                        rctdl_atm_val atom = pAtomElem->commitOldest();
                        // if address and context do instruction trace follower.
                        // otherwise skip atom and reduce committed elements
                        if(!m_need_ctxt && !m_need_addr)
                        {
                            resp = processAtom(atom,bContProcess);
                        }
                        m_P0_commit--; // mark committed 
                    }
                    if(!pAtomElem->isEmpty())   
                        bPopElem = false;   // don't remove if still atoms to process.
                }
                }
                break;

            case P0_EXCEP:
                resp = processException();  // output trace + exception elements.
                m_P0_commit--;
                break;

            case P0_EXCEP_RET:
                m_output_elem.setType(RCTDL_GEN_TRC_ELEM_EXCEPTION_RET);
                resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
                if(pElem->isP0()) // are we on a core that counts ERET as P0?
                    m_P0_commit--;
                break;
            }

            if(bPopElem)
                m_P0_stack.pop_back();  // remove element from stack;

            // if response not continue, then break out of the loop.
            if(!RCTDL_DATA_RESP_IS_CONT(resp))
            {
                bPause = true;
            }
        }
        else
        {
            // too few elements for commit operation - decode error.
            rctdl_trc_index_t err_idx = 0;
            if(pElem)
                err_idx = pElem->getRootIndex();
              
            resp = RCDTL_RESP_FATAL_INVALID_DATA;
            LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_COMMIT_PKT_OVERRUN,err_idx,m_CSID,"Not enough elements to commit"));
            bPause = true;
        }
    }

    // done all elements - need more packets.
    if(m_P0_commit == 0)    
        m_curr_state = DECODE_PKTS;

    // reduce the spec depth by number of comitted elements
    m_curr_spec_depth -= (num_commit_req-m_P0_commit);

    return resp;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::processAtom(const rctdl_atm_val atom, bool &bCont)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    TrcStackElem *pElem = m_P0_stack.back();  // get the atom element
    bool bWPFound = false;
    rctdl_err_t err;
    bCont = true;

    err = traceInstrToWP(bWPFound);
    if(err != RCTDL_OK)
    {
        // TBD: set up error handling
        if(err == RCTDL_ERR_UNSUPPORTED_ISA)
        {
             m_need_addr = true;
             m_need_ctxt = true;
             // wait for next context
             return resp;
        }
        else
        {
            bCont = false;
            resp = RCDTL_RESP_FATAL_INVALID_DATA;
            LogError(rctdlError(RCTDL_ERR_SEV_ERROR,err,pElem->getRootIndex(),m_CSID,"Error processing atom packet."));  
            return resp;
        }
    }

    if(bWPFound)
    {
        // action according to waypoint type and atom value
        switch(m_instr_info.type)
        {
        case RCTDL_INSTR_BR:
            if(atom == ATOM_E)
                m_instr_info.instr_addr = m_instr_info.branch_addr;
            break;

        case RCTDL_INSTR_BR_INDIRECT:
            if(atom == ATOM_E)
                m_need_addr = true; // indirect branch taken - need new address.
            break;
        }
        m_output_elem.setType(RCTDL_GEN_TRC_ELEM_INSTR_RANGE);
        m_output_elem.last_instr_exec = (atom == ATOM_E) ? 1 : 0;
        m_output_elem.last_i_type = m_instr_info.type;
        m_output_elem.i_type_with_link = m_instr_info.is_link;
        resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);

    }
    else
    {
        // no waypoint - likely inaccessible memory range.
        m_need_addr = true; // need an address update 

        if(m_output_elem.st_addr != m_output_elem.en_addr)
        {
            // some trace before we were out of memory access range
            m_output_elem.setType(RCTDL_GEN_TRC_ELEM_INSTR_RANGE);
            resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
        }

        if(m_mem_nacc_pending && RCTDL_DATA_RESP_IS_CONT(resp))
        {
            m_output_elem.setType(RCTDL_GEN_TRC_ELEM_ADDR_NACC);
            m_output_elem.st_addr = m_nacc_addr;
            resp = outputTraceElementIdx(pElem->getRootIndex(),m_output_elem);
            m_mem_nacc_pending = false;
        }
    }

    if(!RCTDL_DATA_RESP_IS_CONT(resp))
        bCont = false;

    return resp;
}

rctdl_datapath_resp_t  TrcPktDecodeEtmV4I::processException()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;   
    TrcStackElemExcept *pExceptElem = dynamic_cast<TrcStackElemExcept *>(m_P0_stack.back());  // get the exception element
    TrcStackElemAddr *pAddressElem = 0;
    TrcStackElemCtxt *pCtxtElem = 0;
    TrcStackElem *pElem = 0;

    m_P0_stack.pop_back(); // remove the exception element
    pElem = m_P0_stack.back();  // look at next element.
    if(pElem->getP0Type() == P0_CTXT)
    {
        pCtxtElem = dynamic_cast<TrcStackElemCtxt *>(pElem);
        m_P0_stack.pop_back(); // remove the context element
        pElem = m_P0_stack.back();  // next one should be an address element
    }
   
    if(pElem->getP0Type() != P0_ADDR)
    {
        // no following address element - indicate processing error.
        resp = RCDTL_RESP_FATAL_INVALID_DATA;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_BAD_PACKET_SEQ,pExceptElem->getRootIndex(),m_CSID,"Address missing in exception packet."));  
    }
    else
    {
        // extract address
        pAddressElem = static_cast<TrcStackElemAddr *>(pElem);
        etmv4_addr_val_t addr;
        int match_idx;
        if(pAddressElem->isAddrMatch(match_idx))
        {
            addr = m_pAddrRegs->get(match_idx);
        }
        else
        {
            // if 64 bit value, or IAsize is 32 bit only...
            if(!m_IASize64 || pAddressElem->is64bit())
            {
                // use value immediately
                addr = pAddressElem->getAddr();                
            }
            else
            {
                // otherwise 32 bit values must add in the top 32 from the stack
                addr = m_pAddrRegs->get(0);
                addr.val &= ~((rctdl_vaddr_t)0xFFFFFFFF);
                addr.val |= (pAddressElem->getAddr().val & 0xFFFFFFFF);
                addr.isa = pAddressElem->getAddr().isa;
            }                                                                                                
        }

        // if we have context, get that.
        if(pCtxtElem)
             updateContext(pCtxtElem);

        // if implied same as previous P0 target then add to instruction start address,
        if(pExceptElem->getPrevSame())
        {
            SetInstrInfoInAddrISA(addr.val,addr.isa);
            m_output_elem.isa = m_instr_info.isa;
        }

        // last instr_info address is the start address - which may now also be the pref ret addr.
        m_output_elem.st_addr = m_instr_info.instr_addr;

        // add end address as preferred return address to end addr in element
        m_output_elem.en_addr = addr.val;

        if(!pAddressElem->isAddrMatch(match_idx))
            m_pAddrRegs->push(addr);
        
        m_output_elem.gen_value = pExceptElem->getExcepNum();

        // output element.
        m_output_elem.setType(RCTDL_GEN_TRC_ELEM_EXCEPTION);
        resp = outputTraceElementIdx(pExceptElem->getRootIndex(),m_output_elem);        
    }   
    return resp;
}

void TrcPktDecodeEtmV4I::SetInstrInfoInAddrISA(const rctdl_vaddr_t addr_val, const uint8_t isa)
{
    m_instr_info.instr_addr = addr_val;
    if(m_is_64bit)
        m_instr_info.isa = rctdl_isa_aarch64;
    else
        m_instr_info.isa = (isa == 0) ? rctdl_isa_arm : rctdl_isa_thumb2;
}

// trace an instruction range to a waypoint - and set next address to restart from.
rctdl_err_t TrcPktDecodeEtmV4I::traceInstrToWP(bool &bWPFound)
{
    uint32_t opcode;
    uint32_t bytesReq;
    rctdl_err_t err = RCTDL_OK;

    // TBD: update mem space to allow for EL as well.
    rctdl_mem_space_acc_t mem_space = m_is_secure ? RCTDL_MEM_SPACE_S : RCTDL_MEM_SPACE_N;

    m_output_elem.st_addr = m_output_elem.en_addr = m_instr_info.instr_addr;

    bWPFound = false;

    while(!bWPFound && !m_mem_nacc_pending)
    {
        // start off by reading next opcode;
        bytesReq = 4;
        err = accessMemory(m_instr_info.instr_addr,mem_space,&bytesReq,(uint8_t *)&opcode);
        if(err != RCTDL_OK) break;

        if(bytesReq == 4) // got data back
        {
            m_instr_info.opcode = opcode;
            err = instrDecode(&m_instr_info);
            if(err != RCTDL_OK) break;

            // increment address - may be adjusted by direct branch value later
            m_instr_info.instr_addr += m_instr_info.instr_size;

            // update the range decoded address in the output packet.
            m_output_elem.en_addr = m_instr_info.instr_addr;

            bWPFound = (m_instr_info.type != RCTDL_INSTR_OTHER);
        }
        else
        {
            // not enough memory accessible.
            m_mem_nacc_pending = true;
            m_nacc_addr = m_instr_info.instr_addr;
        }
    }
    return err;
}

void TrcPktDecodeEtmV4I::updateContext(TrcStackElemCtxt *pCtxtElem)
{
    etmv4_context_t ctxt = pCtxtElem->getContext();
    // map to output element  and local saved state.
    m_is_64bit = (ctxt.SF != 0);
    m_output_elem.context.bits64 = ctxt.SF;
    m_is_secure = (ctxt.NS == 0);
    m_output_elem.context.security_level = ctxt.NS ? rctdl_sec_nonsecure : rctdl_sec_secure;
    m_output_elem.context.exception_level = (rctdl_ex_level)ctxt.EL;
    m_output_elem.context.el_valid = 1;
    if(ctxt.updated_c)
    {
        m_output_elem.context.ctxt_id_valid = 1;
        m_context_id = m_output_elem.context.context_id = ctxt.ctxtID;
    }
    if(ctxt.updated_v)
    {
        m_output_elem.context.vmid_valid = 1;
        m_vmid_id = m_output_elem.context.vmid = ctxt.VMID;
    }
    m_need_ctxt = false;
}

rctdl_datapath_resp_t TrcPktDecodeEtmV4I::handleBadPacket(const char *reason)
{
    rctdl_datapath_resp_t resp  = RCTDL_RESP_CONT;   

    if(getComponentOpMode() && RCTDL_OPFLG_PKTDEC_ERROR_BAD_PKTS)
    {
        // error out - stop decoding
        resp = RCDTL_RESP_FATAL_INVALID_DATA;
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_BAD_DECODE_PKT,reason));
    }
    else
    {
        // switch to unsync - clear decode state
        m_output_elem.setType(RCTDL_GEN_TRC_ELEM_NO_SYNC);
        resp = outputTraceElement(m_output_elem);
        resetDecoder();
        m_curr_state = WAIT_SYNC;
    }
    return resp;
}

/* End of File trc_pkt_decode_etmv4i.cpp */

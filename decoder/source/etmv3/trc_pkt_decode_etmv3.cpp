/*!
 * \file       trc_pkt_decode_etmv3.cpp
 * \brief      OpenCSD : ETMv3 trace packet decode.
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
ocsd_datapath_resp_t TrcPktDecodeEtmV3::processPacket()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    bool bPktDone = false;

    if(!m_config)
        return OCSD_RESP_FATAL_NOT_INIT;

    // iterate round the state machine, waiting for sync, then decoding packets.
    while(!bPktDone)
    {
        switch(m_curr_state)
        {
        case NO_SYNC:
            // output the initial not synced packet to the sink
            m_output_elem.setType(OCSD_GEN_TRC_ELEM_NO_SYNC);
            resp = outputTraceElement(m_output_elem);
            m_curr_state = WAIT_ASYNC;
            break;

        case WAIT_ASYNC:
            if(m_curr_packet_in->getType() == ETM3_PKT_A_SYNC)
                m_curr_state = WAIT_ISYNC;
            bPktDone = true;
            break;

        case WAIT_ISYNC:
            if((m_curr_packet_in->getType() == ETM3_PKT_I_SYNC) || 
                (m_curr_packet_in->getType() == ETM3_PKT_I_SYNC_CYCLE))
                m_curr_state = DECODE_PKTS;
            else
                bPktDone = true;    // not I-sync - done.
            break;

        case DECODE_PKTS:
            resp = decodePacket();
            bPktDone = true;
            break;

        case PEND_INSTR:
            // check the current packet for cancel 
            if(!m_curr_packet_in->isExcepCancel())
                // if not output the last instruction
                resp = outputTraceElement(m_output_elem);    
            
            if( OCSD_DATA_RESP_IS_CONT(resp))
                 m_curr_state = DECODE_PKTS;
            else
            {
                 pendPacket();
                 bPktDone = true;
            }
            break;

        case PEND_PACKET:
            {
                const EtmV3TrcPacket *p_temp_pkt = m_curr_packet_in;
                m_curr_packet_in = &m_pended_packet;
                m_curr_state = DECODE_PKTS;
                resp = decodePacket();
                m_curr_packet_in = p_temp_pkt;
                if(!OCSD_DATA_RESP_IS_CONT(resp))
                {
                    // previous packet pended returned a wait, so pend current
                    pendPacket();
                    bPktDone = true;
                }
            }
            break;

        }
    }

    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeEtmV3::onEOT()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    
    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeEtmV3::onReset()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    resetDecoder();
    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeEtmV3::onFlush()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    return resp;
}

ocsd_err_t TrcPktDecodeEtmV3::onProtocolConfig()
{
    ocsd_err_t err = OCSD_OK;
    if(m_config)
    {
        // set some static config elements
        m_CSID = m_config->getTraceID();

        // check config compatible with current decoder support level.
        // at present no data trace;
        if(m_config->GetTraceMode() != EtmV3Config::TM_INSTR_ONLY)
        {
            err = OCSD_ERR_HW_CFG_UNSUPP;
            LogError(ocsdError(OCSD_ERR_SEV_ERROR,OCSD_ERR_HW_CFG_UNSUPP,"ETMv3 trace decoder : data trace decode not yet supported"));
        }

        // need to set up core profile info in follower
        ocsd_arch_profile_t arch_profile;
        arch_profile.arch = m_config->getArchVersion();
        arch_profile.profile = m_config->getCoreProfile();
        m_code_follower.setArchProfile(arch_profile);
        m_code_follower.setMemSpaceCSID(m_CSID);
    }
    else
        err = OCSD_ERR_NOT_INIT;
    return err;
}

/* local decode methods */

// initialise on creation
void TrcPktDecodeEtmV3::initDecoder()
{
    m_CSID = 0;
    resetDecoder();
    m_code_follower.initInterfaces(getMemoryAccessAttachPt(),getInstrDecodeAttachPt());
}

// reset for first use / re-use.
void TrcPktDecodeEtmV3::resetDecoder()
{
    m_curr_state = NO_SYNC; // mark as not synced
    m_pe_context.resetCtxt();
    m_output_elem.init();

}

// simple packet transforms handled here, more complex processing passed on to specific routines.
ocsd_datapath_resp_t TrcPktDecodeEtmV3::decodePacket()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    bool bOutputElem = false;
    bool bISyncHasCC = false;

    switch(m_curr_packet_in->getType())
    {

    case ETM3_PKT_NOTSYNC:
        // mark as not synced - must have lost sync in the packet processor somehow
        LogError(ocsdError(OCSD_ERR_SEV_ERROR,OCSD_ERR_BAD_PACKET_SEQ,m_index_curr_pkt,m_CSID,"Trace Packet Synchronisation Lost"));
        resetDecoder(); // mark decoder as unsynced - dump any current state.
        break;

        // no action for these packets - ignore and continue
    case ETM3_PKT_INCOMPLETE_EOT:
    case ETM3_PKT_A_SYNC:
    case ETM3_PKT_IGNORE:
        break;

// markers for valid packets
    case ETM3_PKT_CYCLE_COUNT:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_CYCLE_COUNT);
        m_output_elem.setCycleCount(m_curr_packet_in->getCycleCount());
        bOutputElem = true;
        break;

    case ETM3_PKT_TRIGGER:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_EVENT);
        m_output_elem.setEvent(EVENT_TRIGGER,0);
        bOutputElem = true;
        break;

    case ETM3_PKT_BRANCH_ADDRESS:
        resp = processBranchAddr();
        break;

    case ETM3_PKT_I_SYNC_CYCLE:
        bISyncHasCC = true;
    case ETM3_PKT_I_SYNC:
        resp = processISync(bISyncHasCC);
        break;

    case ETM3_PKT_P_HDR:
        resp = processPHdr();
        break;

    case ETM3_PKT_CONTEXT_ID:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_PE_CONTEXT);
        m_pe_context.setCtxtID(m_curr_packet_in->getCtxtID());
        m_output_elem.setContext(m_pe_context);
        bOutputElem = true;
        break;

    case ETM3_PKT_VMID:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_PE_CONTEXT);
        m_pe_context.setVMID(m_curr_packet_in->getVMID());
        m_output_elem.setContext(m_pe_context);
        bOutputElem = true;
        break;

    case ETM3_PKT_EXCEPTION_ENTRY:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_EXCEPTION);
        m_output_elem.setExcepMarker(); // exception entries are always v7M data markers in ETMv3 trace.
        bOutputElem = true;
        break;

    case ETM3_PKT_EXCEPTION_EXIT:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_EXCEPTION_RET);
        bOutputElem = true;
        break;
        
    case ETM3_PKT_TIMESTAMP:
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_TIMESTAMP);
        m_output_elem.setTS(m_curr_packet_in->getTS());
        bOutputElem = true;
        break;

        // data packets - data trace not supported at present
    case ETM3_PKT_STORE_FAIL:
    case ETM3_PKT_OOO_DATA:
    case ETM3_PKT_OOO_ADDR_PLC:
    case ETM3_PKT_NORM_DATA:
    case ETM3_PKT_DATA_SUPPRESSED:
    case ETM3_PKT_VAL_NOT_TRACED:
    case ETM3_PKT_BAD_TRACEMODE:
        LogError(ocsdError(OCSD_ERR_SEV_ERROR,OCSD_ERR_HW_CFG_UNSUPP,m_index_curr_pkt,m_CSID,"Invalid packet type : Data Tracing decode not supported."));
        resp = OCSD_RESP_FATAL_INVALID_DATA;
        resetDecoder(); // mark decoder as unsynced - dump any current state.
        break;


// packet errors 
    case ETM3_PKT_BAD_SEQUENCE:
        LogError(ocsdError(OCSD_ERR_SEV_ERROR,OCSD_ERR_BAD_PACKET_SEQ,m_index_curr_pkt,m_CSID,"Bad Packet sequence."));
        resp = OCSD_RESP_FATAL_INVALID_DATA;
        resetDecoder(); // mark decoder as unsynced - dump any current state.
        break;

    default:
    case ETM3_PKT_RESERVED:
        LogError(ocsdError(OCSD_ERR_SEV_ERROR,OCSD_ERR_BAD_PACKET_SEQ,m_index_curr_pkt,m_CSID,"Reserved or unknown packet ID."));
        resp = OCSD_RESP_FATAL_INVALID_DATA;
        resetDecoder(); // mark decoder as unsynced - dump any current state.
        break;
    }    

    if(bOutputElem)
        resp = outputTraceElementIdx(m_index_curr_pkt,m_output_elem);

    return resp;
}

void TrcPktDecodeEtmV3::pendPacket()
{
    m_pended_packet = *m_curr_packet_in;
    m_curr_state = PEND_PACKET;
}
    
ocsd_datapath_resp_t TrcPktDecodeEtmV3::processISync(const bool withCC)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    // look for context changes....
    if(m_curr_packet_in->isCtxtIDUpdated())
    {

    }

    if(m_config->CtxtIDBytes())
    {

    }

    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeEtmV3::processBranchAddr()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeEtmV3::processPHdr()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    return resp;
}


/* End of File trc_pkt_decode_etmv3.cpp */

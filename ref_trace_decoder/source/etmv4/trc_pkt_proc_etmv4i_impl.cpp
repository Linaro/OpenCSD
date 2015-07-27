/*
 * \file       trc_pkt_proc_etmv4i_impl.cpp
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

#include "trc_pkt_proc_etmv4i_impl.h"

EtmV4IPktProcImpl::EtmV4IPktProcImpl() :
    m_isInit(false),
    m_first_trace_info(false),
    m_interface(0)
{
    BuildIPacketTable();
}

EtmV4IPktProcImpl::~EtmV4IPktProcImpl()
{
}

void EtmV4IPktProcImpl::Initialise(TrcPktProcEtmV4I *p_interface)
{
     if(p_interface)
     {
         m_interface = p_interface;
         m_isInit = true;
     }
     InitProcessorState();
}

rctdl_err_t EtmV4IPktProcImpl::Configure(const EtmV4Config *p_config)
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

rctdl_datapath_resp_t EtmV4IPktProcImpl::processData(  const rctdl_trc_index_t index,
                                    const uint32_t dataBlockSize,
                                    const uint8_t *pDataBlock,
                                    uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    uint32_t bytesProcessed = 0;
    uint8_t currByte;
    while((bytesProcessed < dataBlockSize) && RCTDL_DATA_RESP_IS_CONT(resp))
    {
        currByte = pDataBlock[bytesProcessed];
        try 
        {
            switch(m_process_state)
            {
            case PROC_HDR:
                m_packet_index = index +  bytesProcessed;
                if(m_is_sync)
                {
                    m_pIPktFn = m_i_table[currByte].pptkFn;
                    m_curr_packet.type = m_i_table[currByte].pkt_type;
                }
                else
                {
                    m_pIPktFn = &EtmV4IPktProcImpl::iNotSync;
                    m_curr_packet.type = ETM4_PKT_I_NOTSYNC;
                }
                m_currPacketData.push_back(pDataBlock[bytesProcessed]);
                m_process_state = PROC_DATA;

            case PROC_DATA:
                bytesProcessed++;
                (this->*m_pIPktFn)();                
                break;

            case SEND_PKT:
                resp =  outputPacket();
                InitPacketState();
                m_process_state = PROC_HDR;
                break;
            }
        }
        catch(rctdlError &err)
        {
            m_interface->LogError(err);
            

            /// TBD - determine what to do with the error - depends on error and opmode.
        }
        catch(...)
        {
            /// vv bad at this point.
            resp = RCTDL_RESP_FATAL_SYS_ERR;
            const rctdlError &fatal = rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_FAIL,m_packet_index,m_chanIDCopy,"Unknown System Error decoding trace.");
            m_interface->LogError(fatal);
        }
    }

    *numBytesProcessed = bytesProcessed;
    return resp;
}

rctdl_datapath_resp_t EtmV4IPktProcImpl::onEOT()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    //** TBD:
    return resp;
}

rctdl_datapath_resp_t EtmV4IPktProcImpl::onReset()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    InitProcessorState();
    return resp;
}

rctdl_datapath_resp_t EtmV4IPktProcImpl::onFlush()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    //** TBD:
    return resp;
}

void EtmV4IPktProcImpl::InitPacketState()
{
    m_currPacketData.clear();
    m_curr_packet.initNextPacket(); // clear for next packet.
}

void EtmV4IPktProcImpl::InitProcessorState()
{
    InitPacketState();
    m_pIPktFn = &EtmV4IPktProcImpl::iNotSync;
    m_packet_index = 0;
    m_is_sync = false;
    m_first_trace_info = false;
    m_process_state = PROC_HDR;
    m_curr_packet.initStartState();
}

rctdl_datapath_resp_t EtmV4IPktProcImpl::outputPacket()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    //** TBD:

    
    return resp;
}

void EtmV4IPktProcImpl::outputUnsyncedRawPacket(int nbytes)
{
    //** TBD:
}

void EtmV4IPktProcImpl::iNotSync()
{
    uint8_t lastByte = m_currPacketData.back(); // peek at the byte being processed...

    // is it an extension byte? 
    if(lastByte == 0x00) // TBD : add check for forced sync in here?
    {
        if(m_currPacketData.size() > 1)
        {
            m_currPacketData.pop_back();
            outputUnsyncedRawPacket(m_currPacketData.size());
            m_currPacketData.push_back(lastByte);
        }
        m_pIPktFn = m_i_table[lastByte].pptkFn; 
        (this->*m_pIPktFn)();   // pass on to next analysis fn
    }
    else if(m_currPacketData.size() >= 8)
    {
        outputUnsyncedRawPacket(m_currPacketData.size());
    }
}

void EtmV4IPktProcImpl::iPktNoPayload()
{
    // some expansion may be required...
    uint8_t lastByte = m_currPacketData.back();
    switch(m_curr_packet.type)
    {
    case ETM4_PKT_I_ADDR_MATCH:
        m_curr_packet.setAddressExactMatch(lastByte & 0x3);
        break;

    case ETM4_PKT_I_EVENT:
        m_curr_packet.setEvent(lastByte & 0xF);
        break;

    case ETM4_PKT_I_NUM_DS_MKR:
    case ETM4_PKT_I_UNNUM_DS_MKR:
        m_curr_packet.setDataSyncMarker(lastByte & 0x7);
        break;

    // these just need the packet type - no processing required.
    case ETM4_PKT_I_COND_FLUSH:
    case ETM4_PKT_I_EXCEPT_RTN:
    case ETM4_PKT_I_TRACE_ON:
    default: break;
    }
    m_process_state = SEND_PKT; // now just send it....
}

void EtmV4IPktProcImpl::iPktReserved()
{
    m_process_state = SEND_PKT;
    //** TBD: log error for reserved packet

}

void EtmV4IPktProcImpl::iPktExtension()
{
    uint8_t lastByte = m_currPacketData.back();
    if(m_currPacketData.size() == 2)
    {
        switch(lastByte)
        {
        case 0x03: // discard packet.
            m_curr_packet.type = ETM4_PKT_I_DISCARD;
            m_process_state = SEND_PKT;
            break;

        case 0x05:
            m_curr_packet.type = ETM4_PKT_I_OVERFLOW;
            m_process_state = SEND_PKT;
            break;

        case 0x00:
            m_curr_packet.type = ETM4_PKT_I_ASYNC;
            m_pIPktFn = &EtmV4IPktProcImpl::iPktASync;  // handle subsequent bytes as async
            break;

        default:
            m_curr_packet.err_type = m_curr_packet.type;
            m_curr_packet.type = ETM4_PKT_I_BAD_SEQUENCE;
            m_process_state = SEND_PKT;
            break;
        }
    }
}

void EtmV4IPktProcImpl::iPktASync()
{
    uint8_t lastByte = m_currPacketData.back();
    if(lastByte != 0x00)
    {
        m_process_state = SEND_PKT;
        if((m_currPacketData.size() != 12) || (lastByte != 0x80))
        {
            m_curr_packet.type = ETM4_PKT_I_BAD_SEQUENCE;
            m_curr_packet.err_type = ETM4_PKT_I_ASYNC;
        }
        else
             m_is_sync = true;  // found a sync packet, mark decoder as synchronised.
    }
    else if(m_currPacketData.size() == 12)
    {
        if(!m_is_sync)
        {
            // if we are not yet synced then ignore extra leading 0x00.
            outputUnsyncedRawPacket(1); // get rid of extra 0x00
        }
        else
        {
            // bad periodic ASYNC sequence.
            m_curr_packet.type = ETM4_PKT_I_BAD_SEQUENCE;
            m_curr_packet.err_type = ETM4_PKT_I_ASYNC;
            m_process_state = SEND_PKT;
        }
    }
}

void EtmV4IPktProcImpl::iPktTraceInfo()
{
    // flags to indicate processing for these sections is complete.
    // immediately true if section not present
    static bool ctrlSect = false;
    static bool infoSect = false;   
    static bool keySect =  false;
    static bool specSect = false;
    static bool cyctSect = false;

    uint8_t lastByte = m_currPacketData.back();
    if(m_currPacketData.size() == 1)
    {
        //clear flags
        ctrlSect = infoSect = keySect = specSect = cyctSect = false;
    }
    else if(m_currPacketData.size() == 2)
    {
        // figure out which sections are absent and set to true;
        infoSect = (bool)((lastByte & 0x1) == 0x0);
        keySect =  (bool)((lastByte & 0x2) == 0x0);
        specSect = (bool)((lastByte & 0x4) == 0x0);
        cyctSect = (bool)((lastByte & 0x8) == 0x0);
        ctrlSect = (bool)((lastByte & 0x80) == 0x0);
    }
    else
    {
        if(ctrlSect)
            ctrlSect = (bool)((lastByte & 0x80) == 0x0);
        else if(infoSect)
            infoSect = (bool)((lastByte & 0x80) == 0x0);
        else if(keySect)
            keySect = (bool)((lastByte & 0x80) == 0x0);
        else if(specSect)
            specSect = (bool)((lastByte & 0x80) == 0x0);
        else if(cyctSect)
            cyctSect = (bool)((lastByte & 0x80) == 0x0);        
    }

    // all sections accounted for?
    if(ctrlSect && infoSect && keySect && specSect && cyctSect)
    {
        int idx = 2;
        uint32_t fieldVal = 0;

        // now need to know which sections to look for...
        infoSect = (bool)((lastByte & 0x1) == 0x1);
        keySect =  (bool)((lastByte & 0x2) == 0x2);
        specSect = (bool)((lastByte & 0x4) == 0x4);
        cyctSect = (bool)((lastByte & 0x8) == 0x8);

        m_curr_packet.clearTraceInfo();

        if(infoSect && (idx < m_currPacketData.size()))
        {
            m_curr_packet.setTraceInfo((uint32_t)m_currPacketData[idx]);
            idx++;
        }
        if(keySect && (idx < m_currPacketData.size()))
        {
            idx += extractContField(m_currPacketData,idx,fieldVal);
            m_curr_packet.setTraceInfoKey(fieldVal);
        }
        if(specSect && (idx < m_currPacketData.size()))
        {
            idx += extractContField(m_currPacketData,idx,fieldVal);
            m_curr_packet.setTraceInfoSpec(fieldVal);
        }
        if(cyctSect && (idx < m_currPacketData.size()))
        {
            idx += extractContField(m_currPacketData,idx,fieldVal);
            m_curr_packet.setTraceInfoCyct(fieldVal);
        }
        m_process_state = SEND_PKT;
        m_first_trace_info = true;
    }
}

void EtmV4IPktProcImpl::iPktTimestamp()
{
    static bool ccount_done = false; // done or not needed 
    static bool ts_done = false;
    static int ts_bytes = 0;

    // save the latest byte
    uint8_t lastByte = m_currPacketData.back();

    // process the header byte
    if(m_currPacketData.size() == 1)
    {
        ccount_done = (bool)((lastByte & 0x1) == 0); // 0 = not present
        ts_done = false;
        ts_bytes = 0;
    }
    else
    {        
        if(!ts_done)
        {
            ts_bytes++;
            ts_done = (ts_bytes == 9) || ((lastByte & 0x80) == 0);
        }
        else if(!ccount_done)
        {
            ccount_done = (bool)((lastByte & 0x80) == 0);
            // TBD: check for oorange ccount - bad packet.
        }
    }

    if(ts_done && ccount_done)
    {        
        int idx = 1;
        uint64_t tsVal;
        int ts_bytes = extractContField64(m_currPacketData, idx, tsVal);
        int ts_bits = ts_bytes < 7 ? ts_bytes * 7 : 64;

        if(!m_curr_packet.pkt_valid.bits.ts_valid && m_first_trace_info)
            ts_bits = 64;   // after trace info, missing bits are all 0.

        m_curr_packet.setTS(tsVal,ts_bits);

       
        if((m_currPacketData[0] & 0x1) == 0x1)
        {
            uint32_t countVal, countMask;
            
            idx += ts_bytes;           
            extractContField(m_currPacketData, idx, countVal, 3);    // only 3 possible count bytes.
            countMask = (((uint32_t)1UL << m_config.ccSize()) - 1); // mask of the CC size
            countVal &= countMask;
            m_curr_packet.setCycleCount(countVal);
        }

        m_process_state = SEND_PKT;
    }
}

void EtmV4IPktProcImpl::iPktException()
{
    uint8_t lastByte = m_currPacketData.back();
    static int expExcepSize = 3;

    switch(m_currPacketData.size())
    {
    case 1: expExcepSize = 3; break;
    case 2: if((lastByte & 0x80) == 0x00)
                expExcepSize = 2; 
            break;
    }

    if(m_currPacketData.size() == expExcepSize)
    {
        uint16_t excep_type =  (m_currPacketData[1] >> 1) & 0x1F;
        uint8_t addr_interp = (m_currPacketData[1] & 0x40) >> 5 | (m_currPacketData[1] & 0x1);
        uint8_t m_fault_pending = 0;

        // extended exception packet (probably M class);
        if(m_currPacketData[1] & 0x80)
        {
            excep_type |= ((uint16_t)m_currPacketData[2] & 0x1F) << 5;
            m_fault_pending = (m_currPacketData[2] >> 5)  & 0x1;
        }
        m_curr_packet.setExceptionInfo(excep_type,addr_interp,m_fault_pending);
        m_process_state = SEND_PKT;

        // allow the standard address packet handlers to process the address packet field for the exception.
    }
}

void EtmV4IPktProcImpl::iPktCycleCntF123()
{
    static  rctdl_etmv4_i_pkt_type format = ETM4_PKT_I_CCNT_F1;
    static bool bCCount_done = false; 
    static bool bHasCount = true;
    static bool bCommit_done = false;

    uint8_t lastByte = m_currPacketData.back();
    if( m_currPacketData.size() == 1)
    {
        bCommit_done = bCCount_done = false; 
        bHasCount = true;
        format = m_curr_packet.type;

        if(format == ETM4_PKT_I_CCNT_F3)
        {
            // no commit section for TRCIDR0.COMMOPT == 1
            if(!m_config.commitOpt1())
            {
                m_curr_packet.setCommitElements(((lastByte >> 2) & 0x3) + 1);
            }
            // TBD: warning of non-valid CC threshold here?
            m_curr_packet.setCycleCount(m_curr_packet.getCCThreshold() + lastByte & 0x3);
            m_process_state = SEND_PKT;
        }
        else if(format == ETM4_PKT_I_CCNT_F1) 
        {
            if((lastByte & 0x1) == 0x1)
            {
                bHasCount = false;
                bCCount_done = true;
            }

            // no commit section for TRCIDR0.COMMOPT == 1
            if(m_config.commitOpt1())
                bCommit_done = true;
        }
    }
    else if((format == ETM4_PKT_I_CCNT_F2) && ( m_currPacketData.size() == 2))
    {
        int commit_offset = ((lastByte & 0x1) == 0x1) ? ((int)m_config.MaxSpecDepth() - 15) : 1;
        int commit_elements = ((lastByte >> 4) & 0xF);
        commit_elements += commit_offset;

        // TBD: warning if commit elements < 0?

        m_curr_packet.setCycleCount(m_curr_packet.getCCThreshold() + lastByte & 0xF);
        m_curr_packet.setCommitElements(commit_elements);
        m_process_state = SEND_PKT;
    }
    else
    {
        // F1 and size 2 or more
        if(!bCommit_done)
            bCommit_done = ((lastByte & 0x80) == 0x00);
        else if(!bCCount_done)
            bCCount_done = ((lastByte & 0x80) == 0x00);
    }

    if((format == ETM4_PKT_I_CCNT_F1) && bCommit_done && bCCount_done)
    {        
        int idx = 1; // index into buffer for payload data.
        uint32_t field_value = 0;
        // no commit section for TRCIDR0.COMMOPT == 1
        if(!m_config.commitOpt1())
        {
            idx += extractContField(m_currPacketData,idx,field_value);
            m_curr_packet.setCommitElements(field_value);
        }
        if(bHasCount)
        {
            extractContField(m_currPacketData,idx,field_value, 3);
            m_curr_packet.setCycleCount(field_value);
        }
        m_process_state = SEND_PKT;
    }
}

void EtmV4IPktProcImpl::iPktSpeclRes()
{
    static  rctdl_etmv4_i_pkt_type format = ETM4_PKT_I_COMMIT;

    uint8_t lastByte = m_currPacketData.back();
    if(m_currPacketData.size() == 1)
    {
        format = m_curr_packet.type;
        switch(format)
        {
        case ETM4_PKT_I_MISPREDICT:
        case ETM4_PKT_I_CANCEL_F2:
            switch(lastByte & 0x3)
            {
            case 0x1: m_curr_packet.setAtomPacket(ATOM_PATTERN, 0x1, 1); break; // E
            case 0x2: m_curr_packet.setAtomPacket(ATOM_PATTERN, 0x3, 2); break; // EE
            case 0x3: m_curr_packet.setAtomPacket(ATOM_PATTERN, 0x0, 1); break; // N
            }
            if(format == ETM4_PKT_I_CANCEL_F2)
                m_curr_packet.setCancelElements(1);
            m_process_state = SEND_PKT;
            break;

        case ETM4_PKT_I_CANCEL_F3:
            if(lastByte & 0x1)
                m_curr_packet.setAtomPacket(ATOM_PATTERN, 0x1, 1); // E
            m_curr_packet.setCancelElements(((lastByte >> 1) & 0x3) + 2);
            m_process_state = SEND_PKT;
            break;
        }        
    }
    else
    {
        if((lastByte & 0x80) == 0x00)
        {
            uint32_t field_val = 0;
            extractContField(m_currPacketData,1,field_val);
            if(format == ETM4_PKT_I_COMMIT)
                m_curr_packet.setCommitElements(field_val);
            else
                m_curr_packet.setCancelElements(field_val);
            // TBD: sanity check with max spec depth here?
            m_process_state = SEND_PKT;
        }
    }
}

void EtmV4IPktProcImpl::iPktCondInstr()   
{
    static rctdl_etmv4_i_pkt_type format = ETM4_PKT_I_COND_I_F1;

    uint8_t lastByte = m_currPacketData.back();
    bool bF1Done = false;

    if(m_currPacketData.size() == 1)    
    {
        format = m_curr_packet.type;
        if(format == ETM4_PKT_I_COND_I_F2)
        {
            m_curr_packet.setCondIF2(lastByte & 0x3);
            m_process_state = SEND_PKT;
        }

    }
    else if(m_currPacketData.size() == 2)  
    {
        if(format == ETM4_PKT_I_COND_I_F3)   // f3 two bytes long
        {
            uint8_t num_c_elem = ((lastByte >> 1) & 0x3F) + lastByte & 0x1;
            m_curr_packet.setCondIF3(num_c_elem,(bool)((lastByte & 0x1) == 0x1));
            // TBD: check for 0 num_c_elem in here.
            m_process_state = SEND_PKT;
        }
        else
        {
            bF1Done = ((lastByte & 0x80) == 0x00);
        }
    }
    else
    {
        bF1Done = ((lastByte & 0x80) == 0x00);
    }

    if(bF1Done)
    {
        uint32_t cond_key = 0;
        extractContField(m_currPacketData, 1, cond_key);       
        m_process_state = SEND_PKT;        
    }
}

void EtmV4IPktProcImpl::iPktCondResult()
{
    static rctdl_etmv4_i_pkt_type format = ETM4_PKT_I_COND_RES_F1; // conditional result formats F1-F4

    static bool bF1P1Done = false;  // F1 payload 1 done
    static bool bF1P2Done = false;  // F1 payload 2 done
    static bool bF1HasP2 = false;   // F1 has a payload 2

    uint8_t lastByte = m_currPacketData.back();
    if(m_currPacketData.size() == 1)    
    {
        format = m_curr_packet.type;
        
        switch(format)
        {
        case ETM4_PKT_I_COND_RES_F1:            
            bF1P1Done = bF1P2Done = false;
            bF1HasP2 = true;
            if((lastByte & 0xFC) == 0x6C)// only one payload set
            {
                bF1P2Done = true;
                bF1HasP2 = false;
            }
            break;

        case ETM4_PKT_I_COND_RES_F2:
            m_curr_packet.setCondRF2((lastByte & 0x4) ? 2 : 1, lastByte & 0x3);
            m_process_state = SEND_PKT;
            break;

        case ETM4_PKT_I_COND_RES_F3:
            break;

        case ETM4_PKT_I_COND_RES_F4:
            m_curr_packet.setCondRF4(lastByte & 0x3);
            m_process_state = SEND_PKT;
            break;
        }        
    }
    else if((format == ETM4_PKT_I_COND_RES_F3) && (m_currPacketData.size() == 2)) 
    {
        // 2nd F3 packet
        uint16_t f3_tokens = 0;
        f3_tokens = (uint16_t)m_currPacketData[1];
        f3_tokens |= ((uint16_t)m_currPacketData[0] & 0xf) << 8;
        m_curr_packet.setCondRF3(f3_tokens);
        m_process_state = SEND_PKT;
    }
    else  // !first packet  - F1
    {
        if(!bF1P1Done)
            bF1P1Done = ((lastByte & 0x80) == 0x00);
        else if(!bF1P2Done)
            bF1P2Done = ((lastByte & 0x80) == 0x00);

        if(bF1P1Done && bF1P2Done)
        {
            // TBD: populate packet and send it
            int st_idx = 1;
            uint32_t key[2];
            uint8_t result[2];
            uint8_t CI[2];

            st_idx+= extractCondResult(m_currPacketData,st_idx,key[0],result[0]);
            CI[0] = m_currPacketData[0] & 0x1;
            if(bF1HasP2) // 2nd payload?
            {
                extractCondResult(m_currPacketData,st_idx,key[1],result[1]);
                CI[1] = (m_currPacketData[0] >> 1) & 0x1;
            }
            m_curr_packet.setCondRF1(key,result,CI,bF1HasP2);
            m_process_state = SEND_PKT;
        }
    }
}

void EtmV4IPktProcImpl::iPktContext()
{
    // count of expected VMID bytes in this packet.
    static int nVMID_bytes = 0;  

    // count of expected CID bytes in this packet.
    static int nCtxtID_bytes = 0; 

    bool bSendPacket = false;
    uint8_t lastByte = m_currPacketData.back();
    if(m_currPacketData.size() == 1) 
    {
        if((lastByte & 0x1) == 0)
        {
            m_curr_packet.setContextInfo(false);    // no update context packet (ctxt same as last time).
            m_process_state = SEND_PKT;
        }
    }
    else if(m_currPacketData.size() == 2) 
    {
        if((lastByte & 0xC0) == 0) // no VMID or CID
        {
            bSendPacket = true;
        }
        else
        {
            nVMID_bytes = ((lastByte & 0x40) == 0x40) ? (m_config.vmidSize()/4) : 0;
            nCtxtID_bytes = ((lastByte & 0x80) == 0x80) ? (m_config.cidSize()/4) : 0;
        }
    }
    else    // 3rd byte onwards
    {
        if(nVMID_bytes > 0)
            nVMID_bytes--;
        else if(nCtxtID_bytes > 0)
            nCtxtID_bytes--;

        if((nCtxtID_bytes == 0) && (nVMID_bytes == 0))
            bSendPacket = true;        
    }

    if(bSendPacket)
    {
        extractAndSetContextInfo(m_currPacketData,1);
        m_process_state = SEND_PKT;
    }
}

void EtmV4IPktProcImpl::extractAndSetContextInfo(const std::vector<uint8_t> &buffer, const int st_idx)
{
    // on input, buffer index points at the info byte - always present
    uint8_t infoByte = m_currPacketData[st_idx];
    m_curr_packet.setContextInfo((infoByte & 0x3) != 0, (infoByte >> 5) & 0x1, (infoByte >> 4) & 0x1);
    
    // see if there are VMID and CID bytes, and how many.
    int nVMID_bytes = ((infoByte & 0x40) == 0x40) ? (m_config.vmidSize()/4) : 0;
    int nCtxtID_bytes = ((infoByte & 0x80) == 0x80) ? (m_config.cidSize()/4) : 0;

    // extract any VMID and CID
    int payload_idx = st_idx+1;
    if(nVMID_bytes)
    {
        uint32_t VMID = 0; 
        for(int i = 0; i < nVMID_bytes; i++)
        {
            VMID |= ((uint32_t)m_currPacketData[i+payload_idx] << i*8);
        }
        payload_idx += nVMID_bytes;
        m_curr_packet.setContextVMID(VMID);
    }

    if(nCtxtID_bytes)
    {
        uint32_t CID = 0; 
        for(int i = 0; i < nCtxtID_bytes; i++)
        {
            CID |= ((uint32_t)m_currPacketData[i+payload_idx] << i*8);
        }        
        m_curr_packet.setContextCID(CID);
    }
}

void EtmV4IPktProcImpl::iPktAddrCtxt()
{
    static int nVMID_bytes = 0;
    static int nCtxtID_bytes = 0;
    static int nAddr_bytes = 0;
    static bool bCtxtInfo_done = false;
    static uint8_t IS = 0;
    static bool b64bit = false;

    uint8_t lastByte = m_currPacketData.back();
    bool bSend = false;

    if( m_currPacketData.size() == 1)    
    {        
        IS = 0;
        nAddr_bytes = 0;
        b64bit = false;

        switch(m_curr_packet.type)
        {
        case ETM4_PKT_I_ADDR_CTXT_L_32IS1:
            IS = 1;
        case ETM4_PKT_I_ADDR_CTXT_L_32IS0:
            nAddr_bytes = 4;
            break;

        case ETM4_PKT_I_ADDR_CTXT_L_64IS1:
            IS = 1;
        case ETM4_PKT_I_ADDR_CTXT_L_64IS0:
            nAddr_bytes = 8;
            b64bit = true;
            break;
        }
        bCtxtInfo_done = false;
        nCtxtID_bytes = 0;
        nVMID_bytes = 0;
    }
    else
    {
        if(nAddr_bytes == 0)
        {
            if(bCtxtInfo_done == false)
            {
                bCtxtInfo_done = true;
                nVMID_bytes = ((lastByte & 0x40) == 0x40) ? (m_config.vmidSize()/4) : 0;
                nCtxtID_bytes = ((lastByte & 0x80) == 0x80) ? (m_config.cidSize()/4) : 0;
            }
            else
            {
                if(nVMID_bytes > 0) 
                    nVMID_bytes--;
                else if(nCtxtID_bytes > 0)
                    nCtxtID_bytes--;
            }
        }
        else
            nAddr_bytes--;

        if((nAddr_bytes == 0) && bCtxtInfo_done && (nVMID_bytes == 0) && (nCtxtID_bytes == 0))
        {
            int st_idx = 1;
            if(b64bit)
            {
                uint64_t val64;
                st_idx+=extract64BitLongAddr(m_currPacketData,st_idx,IS,val64);
                m_curr_packet.set64BitAddress(val64,IS,64);
            }
            else
            {
                uint32_t val32;
                st_idx+=extract32BitLongAddr(m_currPacketData,st_idx,IS,val32);
                m_curr_packet.set32BitAddress(val32,IS,32);
            }
            extractAndSetContextInfo(m_currPacketData,st_idx);
            m_process_state = SEND_PKT;
        }
    }
}

void EtmV4IPktProcImpl::iPktShortAddr()
{
    static uint8_t header = 0;
    static bool addr_done = false;
    static uint8_t IS = 0;

    uint8_t lastByte = m_currPacketData.back();
    if(m_currPacketData.size() == 1)    
    {
        addr_done = false;
        IS = (lastByte == ETM4_PKT_I_ADDR_S_IS0) ? 0 : 1;
    }
    else if(!addr_done)
    {
        addr_done = (m_currPacketData.size() == 3) || ((lastByte & 0x80) == 0x00);
    }

    if(addr_done)
    {
        uint32_t addr_val = 0;
        int bits = 0;

        extractShortAddr(m_currPacketData,1,IS,addr_val,bits);
        m_curr_packet.updateShortAddress(addr_val,IS,bits);
        m_process_state = SEND_PKT;
    }
}

int EtmV4IPktProcImpl::extractShortAddr(const std::vector<uint8_t> &buffer, const int st_idx, const uint8_t IS, uint32_t &value, int &bits)
{
    int IS_shift = (IS == 0) ? 2 : 1;
    int idx = 0;

    bits = 7;   // at least 7 bits
    value = 0;
    value |= ((uint32_t)(buffer[st_idx+idx] & 0x7F)) << IS_shift;
    
    if(m_currPacketData[st_idx+idx] & 0x80)
    {
        idx++;
        value |= ((uint32_t)m_currPacketData[st_idx+idx]) <<  (7 + IS_shift);
        bits += 8;
    }
    idx++;
    bits += IS_shift;
    return idx;
}

void EtmV4IPktProcImpl::iPktLongAddr()    
{
    static int addrBytes = 4;
    static uint8_t header = 0;
    static uint8_t IS = 0;
    static bool b64bit = false;

    if(m_currPacketData.size() == 1)    
    {
        IS = 0;
        b64bit = false;
        switch(m_curr_packet.type)
        {
        case ETM4_PKT_I_ADDR_L_32IS1:
            IS = 1;
        case ETM4_PKT_I_ADDR_L_32IS0:
            addrBytes = 4;
            break;

        case ETM4_PKT_I_ADDR_L_64IS1:
            IS = 1;
        case ETM4_PKT_I_ADDR_L_64IS0:
            addrBytes = 8;
            b64bit = true;
            break;
        }
    }
    if(m_currPacketData.size() == (1+addrBytes))
    {
        int st_idx = 1;
        if(b64bit)
        {
            uint64_t val64;
            st_idx+=extract64BitLongAddr(m_currPacketData,st_idx,IS,val64);
        }
        else
        {
            uint32_t val32;
            st_idx+=extract32BitLongAddr(m_currPacketData,st_idx,IS,val32);
        }
        m_process_state = SEND_PKT;
    }
}

void EtmV4IPktProcImpl::iPktQ()
{
    static int addrBytes = 0;
    static bool has_addr = false;
    static bool count_done = false;
    static bool addr_short = false;
    static bool addr_match = false;
    static uint8_t q_type = 0;
    static uint8_t IS = 0;
    static uint8_t QE = 0;

    uint8_t lastByte = m_currPacketData.back();
    bool bSendBad = false;
    if(m_currPacketData.size() == 1)
    {
        q_type = lastByte & 0xF;

        addrBytes = 0;
        count_done = false;
        has_addr = false;
        addr_short = true;
        addr_match = false;
        IS = 1;
        QE = 0;

        switch(q_type)
        {
            // count only - implied address.
        case 0x0:
        case 0x1:
        case 0x2:
            addr_match = true;
            has_addr = true;
            QE = q_type & 0x3;
        case 0xC:
            break;

            // count + short address 
        case 0x5:
            IS = 0;
        case 0x6:
            has_addr = true;            
            addrBytes = 2;  // short IS0/1
            break;

            // count + long address
        case 0xA:
            IS = 0;
        case 0xB:
            has_addr = true;
            addr_short = false;
            addrBytes = 4; // long IS0/1
            break;

            // no count, no address
        case 0xF:
            count_done = true;
            break;

            // reserved values 0x3, 0x4, 0x7, 0x8, 0x9, 0xD, 0xE
        default:
            m_curr_packet.err_type =  m_curr_packet.type;
            m_curr_packet.type = ETM4_PKT_I_BAD_SEQUENCE;
            //SendBadIPacket( PKT_BAD_SEQUENCE, "ERROR: Bad Q packet type", PKT_Q );
            break;
        }
    }
    else
    {
        if(addrBytes > 0)
        {
            if(addr_short && addrBytes == 2)  // short
            {
                if((lastByte & 0x80) == 0x00)
                    addrBytes--;        // short version can have just single byte.
            }
            addrBytes--;
        }
        else if(!count_done)
        {
            count_done = ((lastByte & 0x80) == 0x00);
        }
    }

    if(((addrBytes == 0) && count_done))
    {
        int idx = 1; // move past the header
        int bits = 0;
        uint32_t q_addr;
        uint32_t q_count;

        if(has_addr)
        {
            if(addr_match)
            {
                m_curr_packet.setAddressExactMatch(QE);
            }
            else if(addr_short)
            {
                idx+=extractShortAddr(m_currPacketData,idx,IS,q_addr,bits);
                m_curr_packet.updateShortAddress(q_addr,IS,bits);
            }
            else
            {
                idx+=extract32BitLongAddr(m_currPacketData,idx,IS,q_addr);
                m_curr_packet.set32BitAddress(q_addr,IS,32);
            }
        }

        if(q_type != 0xF)
        {
            extractContField(m_currPacketData,idx,q_count);
            m_curr_packet.setQType(true,q_count,has_addr,addr_match,q_type);
        }
        else
        {
            m_curr_packet.setQType(false,0,false,false,0xF);
        }
        m_process_state = SEND_PKT;
    }

}

void EtmV4IPktProcImpl::iAtom()
{
    // patterns lsbit = oldest atom, ms bit = newest.
    static const uint32_t f4_patterns[] = {
        0xE, // EEEN 
        0x0, // NNNN
        0xA, // ENEN
        0x5  // NENE
    };

    uint8_t lastByte = m_currPacketData.back();
    uint8_t pattIdx = 0, pattCount = 0;
    uint32_t pattern;

    // atom packets are single byte, no payload.
    switch(m_curr_packet.type)
    {
    case ETM4_PKT_I_ATOM_F1:
        m_curr_packet.setAtomPacket(ATOM_PATTERN,(lastByte & 0x1), 1); // 1xE or N
        break;

    case ETM4_PKT_I_ATOM_F2:
        m_curr_packet.setAtomPacket(ATOM_PATTERN,(lastByte & 0x3), 2); // 2x (E or N)
        break;

    case ETM4_PKT_I_ATOM_F3:
        m_curr_packet.setAtomPacket(ATOM_PATTERN,(lastByte & 0x7), 3); // 2x (E or N)
        break;

    case ETM4_PKT_I_ATOM_F4:
        m_curr_packet.setAtomPacket(ATOM_PATTERN,f4_patterns[(lastByte & 0x3)], 4); // 4 atom pattern
        break; 

    case ETM4_PKT_I_ATOM_F5:
        pattIdx = ((lastByte & 0x20) >> 3) | (lastByte & 0x3);
        switch(pattIdx)
        {
        case 5: // 0b101
            m_curr_packet.setAtomPacket(ATOM_PATTERN,0x1E, 5); // 5 atom pattern EEEEN
            break;

        case 1: // 0b001
            m_curr_packet.setAtomPacket(ATOM_PATTERN,0x00, 5); // 5 atom pattern NNNNN
            break;

        case 2: //0b010
            m_curr_packet.setAtomPacket(ATOM_PATTERN,0x0A, 5); // 5 atom pattern NENEN
            break;

        case 3: //0b011
            m_curr_packet.setAtomPacket(ATOM_PATTERN,0x15, 5); // 5 atom pattern ENENE
            break;

        default:
            // TBD: warn about invalid pattern in here.
            break;
        }
        break;

    case ETM4_PKT_I_ATOM_F6:
        pattCount = (lastByte & 0x1F) + 3;  // count of E's
        // TBD: check 23 or less at this point? 
        pattern = ((uint32_t)0x1 << pattCount) - 1; // set pattern to string of E's
        if((lastByte & 0x20) == 0x00)   // last atom is E?
            pattern |= ((uint32_t)0x1 << pattCount); 
        m_curr_packet.setAtomPacket(ATOM_PATTERN,pattern, pattCount+1);
        break;
    }

    m_process_state = SEND_PKT;
}

// header byte processing is table driven.
void EtmV4IPktProcImpl::BuildIPacketTable()   
{
    // initialise everything as reserved.
    for(int i = 0; i < 256; i++)
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_RESERVED;
        m_i_table[i].pptkFn = &EtmV4IPktProcImpl::iPktReserved;
    }

    // 0x00 - extension 
    m_i_table[0x00].pkt_type = ETM4_PKT_I_EXTENSION;
    m_i_table[0x00].pptkFn   = &EtmV4IPktProcImpl::iPktExtension;

    // 0x01 - Trace info
    m_i_table[0x00].pkt_type = ETM4_PKT_I_TRACE_INFO;
    m_i_table[0x00].pptkFn   = &EtmV4IPktProcImpl::iPktTraceInfo;

    // b0000001x - timestamp
    m_i_table[0x02].pkt_type = ETM4_PKT_I_TIMESTAMP;
    m_i_table[0x02].pptkFn   = &EtmV4IPktProcImpl::iPktTimestamp;
    m_i_table[0x03].pkt_type = ETM4_PKT_I_TIMESTAMP;
    m_i_table[0x03].pptkFn   = &EtmV4IPktProcImpl::iPktTimestamp;

    // b0000 0100 - trace on
    m_i_table[0x04].pkt_type = ETM4_PKT_I_TRACE_ON;
    m_i_table[0x04].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;

    // b0000 0110 - exception 
    m_i_table[0x06].pkt_type = ETM4_PKT_I_EXCEPT;
    m_i_table[0x06].pptkFn   = &EtmV4IPktProcImpl::iPktException;

    // b0000 0111 - exception return 
    m_i_table[0x07].pkt_type = ETM4_PKT_I_EXCEPT_RTN;
    m_i_table[0x07].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;

    // b0000 110x - cycle count f2
    // b0000 111x - cycle count f1
    for(int i = 0; i < 4; i++)
    {
        m_i_table[0x0C+i].pkt_type = (i >= 2) ? ETM4_PKT_I_CCNT_F1 : ETM4_PKT_I_CCNT_F2;
        m_i_table[0x0C+i].pptkFn   = &EtmV4IPktProcImpl::iPktCycleCntF123;
    }

    // b0001 xxxx - cycle count f3
    for(int i = 0; i < 16; i++)
    {
        m_i_table[0x10+i].pkt_type = ETM4_PKT_I_CCNT_F3;
        m_i_table[0x10+i].pptkFn   = &EtmV4IPktProcImpl::iPktCycleCntF123;
    }

    // b0010 0xxx - NDSM
    for(int i = 0; i < 8; i++)
    {
        m_i_table[0x20+i].pkt_type = ETM4_PKT_I_NUM_DS_MKR;
        m_i_table[0x20+i].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;
    }

    // b0010 10xx, b0010 1100 - UDSM
    for(int i = 0; i < 5; i++)
    {
        m_i_table[0x28+i].pkt_type = ETM4_PKT_I_UNNUM_DS_MKR;
        m_i_table[0x28+i].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;
    }

    // b0010 1101 - commit
    m_i_table[0x2D].pkt_type = ETM4_PKT_I_COMMIT;
    m_i_table[0x2D].pptkFn   = &EtmV4IPktProcImpl::iPktSpeclRes;


    // b0010 111x - cancel f1
    for(int i = 0; i < 2; i++)
    {
        // G++ doesn't understand [0x2E+i] so...
        int idx = i + 0x2E;
        m_i_table[idx].pkt_type = ETM4_PKT_I_CANCEL_F1;
        m_i_table[idx].pptkFn   = &EtmV4IPktProcImpl::iPktSpeclRes;
    }

    // b0011 00xx - mis predict
    for(int i = 0; i < 4; i++)
    {
        m_i_table[0x30+i].pkt_type = ETM4_PKT_I_MISPREDICT;
        m_i_table[0x30+i].pptkFn   =  &EtmV4IPktProcImpl::iPktSpeclRes;
    }

    // b0011 01xx - cancel f2
    for(int i = 0; i < 4; i++)
    {
        m_i_table[0x34+i].pkt_type = ETM4_PKT_I_CANCEL_F2;
        m_i_table[0x34+i].pptkFn   =  &EtmV4IPktProcImpl::iPktSpeclRes;
    }

    // b0011 1xxx - cancel f3
    for(int i = 0; i < 8; i++)
    {
        m_i_table[0x38+i].pkt_type = ETM4_PKT_I_CANCEL_F3;
        m_i_table[0x38+i].pptkFn   =  &EtmV4IPktProcImpl::iPktSpeclRes;
    }

    // b0100 000x, b0100 0010 - cond I f2
    for(int i = 0; i < 3; i++)
    {
        m_i_table[0x40+i].pkt_type = ETM4_PKT_I_COND_I_F2;
        m_i_table[0x40+i].pptkFn   = &EtmV4IPktProcImpl::iPktCondInstr;
    }

    // b0100 0011 - cond flush
    m_i_table[0x43].pkt_type = ETM4_PKT_I_COND_FLUSH;
    m_i_table[0x43].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;

    // b0100 010x, b0100 0110 - cond res f4
    for(int i = 0; i < 3; i++)
    {
        m_i_table[0x44+i].pkt_type = ETM4_PKT_I_COND_RES_F4;
        m_i_table[0x44+i].pptkFn   = &EtmV4IPktProcImpl::iPktCondResult;
    }

    // b0100 100x, b0100 0110 - cond res f2
    // b0100 110x, b0100 1110 - cond res f2
    for(int i = 0; i < 3; i++)
    {
        m_i_table[0x48+i].pkt_type = ETM4_PKT_I_COND_RES_F2;
        m_i_table[0x48+i].pptkFn   = &EtmV4IPktProcImpl::iPktCondResult;
    }
    for(int i = 0; i < 3; i++)
    {
        m_i_table[0x4C+i].pkt_type = ETM4_PKT_I_COND_RES_F2;
        m_i_table[0x4C+i].pptkFn   = &EtmV4IPktProcImpl::iPktCondResult;
    }

    // b0101xxxx - cond res f3
    for(int i = 0; i < 16; i++)
    {
        m_i_table[0x50+i].pkt_type = ETM4_PKT_I_COND_RES_F3;
        m_i_table[0x50+i].pptkFn   = &EtmV4IPktProcImpl::iPktCondResult;
    }

    // b011010xx - cond res f1
    for(int i = 0; i < 4; i++)
    {
        m_i_table[0x68+i].pkt_type = ETM4_PKT_I_COND_RES_F1;
        m_i_table[0x68+i].pptkFn   = &EtmV4IPktProcImpl::iPktCondResult;
    }

    // b0110 1100 - cond instr f1
    m_i_table[0x6C].pkt_type = ETM4_PKT_I_COND_I_F1;
    m_i_table[0x6C].pptkFn   = &EtmV4IPktProcImpl::iPktCondInstr;

    // b0110 1101 - cond instr f3
    m_i_table[0x6D].pkt_type = ETM4_PKT_I_COND_I_F3;
    m_i_table[0x6D].pptkFn   = &EtmV4IPktProcImpl::iPktCondInstr;

    // b0110111x - cond res f1
    for(int i = 0; i < 2; i++)
    {
        // G++ cannot understand [0x6E+i] so change these round
        m_i_table[i+0x6E].pkt_type = ETM4_PKT_I_COND_RES_F1;
        m_i_table[i+0x6E].pptkFn   = &EtmV4IPktProcImpl::iPktCondResult;
    }

    // b01110001 - b01111111 - cond res f1
    for(int i = 0; i < 15; i++)
    {
        m_i_table[0x71+i].pkt_type = ETM4_PKT_I_EVENT;
        m_i_table[0x71+i].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;
    }
    
    // 0b1000 000x - context 
    for(int i = 0; i < 2; i++)
    {
        m_i_table[0x80+i].pkt_type = ETM4_PKT_I_CTXT;
        m_i_table[0x80+i].pptkFn   = &EtmV4IPktProcImpl::iPktContext;
    }

    // 0b1000 0010 to b1000 0011 - addr with ctxt
    // 0b1000 0101 to b1000 0110 - addr with ctxt
    for(int i = 0; i < 2; i++)
    {
        m_i_table[0x82+i].pkt_type =  (i == 0) ? ETM4_PKT_I_ADDR_CTXT_L_32IS0 : ETM4_PKT_I_ADDR_CTXT_L_32IS1;
        m_i_table[0x82+i].pptkFn   = &EtmV4IPktProcImpl::iPktAddrCtxt;
    }

    for(int i = 0; i < 2; i++)
    {
        m_i_table[0x85+i].pkt_type = (i == 0) ? ETM4_PKT_I_ADDR_CTXT_L_64IS0 : ETM4_PKT_I_ADDR_CTXT_L_64IS1;
        m_i_table[0x85+i].pptkFn   = &EtmV4IPktProcImpl::iPktAddrCtxt;
    }

    // 0b1001 0000 to b1001 0010 - exact match addr
    for(int i = 0; i < 3; i++)
    {
        m_i_table[0x90+i].pkt_type = ETM4_PKT_I_ADDR_MATCH;
        m_i_table[0x90+i].pptkFn   = &EtmV4IPktProcImpl::iPktNoPayload;
    }

    // b1001 0101 - b1001 0110 - addr short address
    for(int i = 0; i < 2; i++)
    {
        m_i_table[0x95+i].pkt_type =  (i == 0) ? ETM4_PKT_I_ADDR_S_IS0 : ETM4_PKT_I_ADDR_S_IS1;
        m_i_table[0x95+i].pptkFn   = &EtmV4IPktProcImpl::iPktShortAddr;
    }

    // b10011010 - b10011011 - addr long address 
    // b10011101 - b10011110 - addr long address 
    for(int i = 0; i < 2; i++)
    {
        m_i_table[0x9A+i].pkt_type =  (i == 0) ? ETM4_PKT_I_ADDR_L_32IS0 : ETM4_PKT_I_ADDR_L_32IS1;
        m_i_table[0x9A+i].pptkFn   = &EtmV4IPktProcImpl::iPktLongAddr;
    }
    for(int i = 0; i < 2; i++)
    {
        m_i_table[0x9D+i].pkt_type =  (i == 0) ? ETM4_PKT_I_ADDR_CTXT_L_64IS0 : ETM4_PKT_I_ADDR_CTXT_L_64IS1;
        m_i_table[0x9D+i].pptkFn   = &EtmV4IPktProcImpl::iPktLongAddr;
    }

    // b1010xxxx - Q packet
    for(int i = 0; i < 16; i++)
    {
        m_i_table[0xA0+i].pkt_type = ETM4_PKT_I_Q;
        m_i_table[0xA0+i].pptkFn   = &EtmV4IPktProcImpl::iPktQ;
    }

    // Atom Packets - all no payload but have specific pattern generation fn
    for(int i = 0xC0; i <= 0xD4; i++)   // atom f6
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F6;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
    for(int i = 0xD5; i <= 0xD7; i++)  // atom f5
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F5;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
    for(int i = 0xD8; i <= 0xDB; i++)  // atom f2
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F2;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
    for(int i = 0xDC; i <= 0xDF; i++)  // atom f4
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F4;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
    for(int i = 0xE0; i <= 0xF4; i++)  // atom f6
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F6;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
    
    // atom f5
    m_i_table[0xF5].pkt_type = ETM4_PKT_I_ATOM_F5;
    m_i_table[0xF5].pptkFn   = &EtmV4IPktProcImpl::iAtom;

    for(int i = 0xF6; i <= 0xF7; i++)  // atom f1
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F1;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
    for(int i = 0xF8; i <= 0xFF; i++)  // atom f3
    {
        m_i_table[i].pkt_type = ETM4_PKT_I_ATOM_F3;
        m_i_table[i].pptkFn   = &EtmV4IPktProcImpl::iAtom;
    }
}

int EtmV4IPktProcImpl::extractContField(const std::vector<uint8_t> &buffer, const int st_idx, uint32_t &value, const int byte_limit /*= 5*/)
{
    int idx = 0;
    bool lastByte = false;
    uint8_t byteVal;
    value = 0;
    while(!lastByte && (idx < byte_limit))   // max 5 bytes for 32 bit value;
    {
        if(buffer.size() < (st_idx + idx))
        {
            // each byte has seven bits + cont bit
            byteVal = buffer[(st_idx + idx)];
            lastByte = (byteVal & 0x80) == 0x80;
            value |= ((uint32_t)byteVal) << (idx * 7);
            idx++;
        }
        else
        {
            // TBD: error out here.
        }
    }
    return idx;
}

int EtmV4IPktProcImpl::extractContField64(const std::vector<uint8_t> &buffer, const int st_idx, uint64_t &value, const int byte_limit /*= 9*/)
{
    int idx = 0;
    bool lastByte = false;
    uint8_t byteVal;
    value = 0;
    while(!lastByte && (idx < byte_limit))   // max 9 bytes for 32 bit value;
    {
        if(buffer.size() < (st_idx + idx))
        {
            // each byte has seven bits + cont bit
            byteVal = buffer[(st_idx + idx)];
            lastByte = (byteVal & 0x80) == 0x80;
            value |= ((uint64_t)byteVal) << (idx * 7);
            idx++;
        }
        else
        {
            // TBD: error out here.
        }
    }
    return idx;
}

int EtmV4IPktProcImpl::extractCondResult(const std::vector<uint8_t> &buffer, const int st_idx, uint32_t& key, uint8_t &result)
{
    int idx = 0;
    bool lastByte = false;
    int incr = 0;

    key = 0;

    while(!lastByte && (idx < 6)) // cannot be more than 6 bytes for res + 32 bit key
    {
        if(buffer.size() < (st_idx + idx))
        {
            if(idx == 0)
            {
                result = buffer[st_idx+idx];
                key = (buffer[st_idx+idx] >> 4) & 0x7;
                incr+=3;
            }
            else
            {
                key |= ((uint32_t)(buffer[st_idx+idx] & 0x7F)) << incr;
                incr+=7;
            }
            idx++;
            lastByte = (bool)((buffer[st_idx+idx] & 0x80) == 0); 
        }
        else
        {
            // TBD: error out here.
        }
    }    
    return idx;
}

int EtmV4IPktProcImpl::extract64BitLongAddr(const std::vector<uint8_t> &buffer, const int st_idx, const uint8_t IS, uint64_t &value)
{
    value = 0;
    if(IS == 0)
    {
        value |= ((uint64_t)(buffer[st_idx+0] & 0x7F)) << 2;
        value |= ((uint64_t)(buffer[st_idx+1] & 0x7F)) << 9;
    }
    else
    {
        value |= ((uint64_t)(buffer[st_idx+0] & 0x7F)) << 1;
        value |= ((uint64_t)buffer[st_idx+1]) << 8;
    }
    value |= ((uint64_t)buffer[st_idx+2]) << 16;
    value |= ((uint64_t)buffer[st_idx+3]) << 24;
    value |= ((uint64_t)buffer[st_idx+4]) << 32;
    value |= ((uint64_t)buffer[st_idx+5]) << 40;
    value |= ((uint64_t)buffer[st_idx+6]) << 48;
    value |= ((uint64_t)buffer[st_idx+7]) << 56;      
    return 4;    
}

int EtmV4IPktProcImpl::extract32BitLongAddr(const std::vector<uint8_t> &buffer, const int st_idx, const uint8_t IS, uint32_t &value)
{
    value = 0;
    if(IS == 0)
    {
        value |= ((uint32_t)(buffer[st_idx+0] & 0x7F)) << 2;
        value |= ((uint32_t)(buffer[st_idx+1] & 0x7F)) << 9;
    }
    else
    {
        value |= ((uint32_t)(buffer[st_idx+0] & 0x7F)) << 1;
        value |= ((uint32_t)buffer[st_idx+1]) << 8;
    }
    value |= ((uint32_t)buffer[st_idx+2]) << 16;
    value |= ((uint32_t)buffer[st_idx+3]) << 24;
    return 4;
}


/* End of File trc_pkt_proc_etmv4i_impl.cpp */

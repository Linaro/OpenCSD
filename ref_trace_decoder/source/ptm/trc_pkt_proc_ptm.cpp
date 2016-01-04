/*
 * \file       trc_pkt_proc_ptm.cpp
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

#include "ptm/trc_pkt_proc_ptm.h"
#include "trc_pkt_proc_ptm_impl.h"
#include "rctdl_error.h"


#ifdef __GNUC__
// G++ doesn't like the ## pasting
#define PTM_PKTS_NAME "PKTP_PTM"
#else
// VC++ is OK
#define PTM_PKTS_NAME RCTDL_CMPNAME_PREFIX_PKTPROC##"_PTM"
#endif

TrcPktProcPtm::TrcPktProcPtm() : TrcPktProcBase(PTM_PKTS_NAME)
{
    InitProcessorState();
}

TrcPktProcPtm::TrcPktProcPtm(int instIDNum) : TrcPktProcBase(PTM_PKTS_NAME, instIDNum)
{
    InitProcessorState();
}

TrcPktProcPtm::~TrcPktProcPtm()
{

}

rctdl_err_t TrcPktProcPtm::onProtocolConfig()
{
    rctdl_err_t err = RCTDL_ERR_NOT_INIT;

    if(m_config != 0)
    {
        m_chanIDCopy = m_config->getTraceID();
        err = RCTDL_OK;
    }
    return err;
}

rctdl_datapath_resp_t TrcPktProcPtm::processData(  const rctdl_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    uint8_t currByte = 0;
    
    m_dataInProcessed = 0;
    
    if(m_config != 0)
    {
        resp = RCTDL_RESP_FATAL_NOT_INIT;
    }
    else
    {
        m_pDataIn = pDataBlock;
        m_dataInLen = dataBlockSize;        
        m_block_idx = index; // index start for current block
    }

    while(  ( ( m_dataInProcessed  < dataBlockSize) || 
              (( m_dataInProcessed  == dataBlockSize) && (m_process_state == SEND_PKT)) ) && 
            RCTDL_DATA_RESP_IS_CONT(resp))
    {
        try
        {
            switch(m_process_state)
            {
            case PROC_HDR:
                m_curr_pkt_index = m_block_idx + m_dataInProcessed;
                if(isSync())
                {
                    if(readByte(currByte))
                    {
                        m_pIPktFn = m_i_table[currByte].pptkFn;
                        m_curr_packet.type = m_i_table[currByte].pkt_type;
                    }
                    else
                    {
                        // sequencing error - should not get to the point where readByte
                        // fails and m_DataInProcessed  < dataBlockSize
                        // throw data overflow error
                        throw rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_PKT_INTERP_FAIL,m_curr_pkt_index,this->m_chanIDCopy,"Data Buffer Overrun");
                    }
                }
                else
                {
                    m_pIPktFn = &TrcPktProcPtm::waitASync;
                    m_curr_packet.type = PTM_PKT_NOTSYNC;
                    
                }
                m_process_state = PROC_DATA;

            case PROC_DATA:
                (this->*m_pIPktFn)();                
                break;

            case SEND_PKT:
                resp = outputPacket();
                InitPacketState();
                m_process_state = PROC_HDR;
                break;
            }
        }
        catch(rctdlError &err)
        {
            LogError(err);
            if( (err.getErrorCode() == RCTDL_ERR_BAD_PACKET_SEQ) ||
                (err.getErrorCode() == RCTDL_ERR_INVALID_PCKT_HDR))
            {
                // send invalid packets up the pipe to let the next stage decide what to do.
                m_process_state = SEND_PKT; 
            }
            else
            {
                // bail out on any other error.
                resp = RCDTL_RESP_FATAL_INVALID_DATA;
            }
        }
        catch(...)
        {
            /// vv bad at this point.
            resp = RCTDL_RESP_FATAL_SYS_ERR;
            const rctdlError &fatal = rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_FAIL,m_curr_pkt_index,m_chanIDCopy,"Unknown System Error decoding trace.");
            LogError(fatal);
        }

    }
    *numBytesProcessed = m_dataInProcessed;
    return resp;
}

rctdl_datapath_resp_t TrcPktProcPtm::onEOT()
{
    rctdl_datapath_resp_t err = RCTDL_RESP_FATAL_NOT_INIT;
    if(m_config != 0)
    {
        err = RCTDL_RESP_CONT;
    }
    return err;
}

rctdl_datapath_resp_t TrcPktProcPtm::onReset()
{
    rctdl_datapath_resp_t err = RCTDL_RESP_FATAL_NOT_INIT;
    if(m_config != 0)
    {
        InitProcessorState();
        err = RCTDL_RESP_CONT;
    }
    return err;
}

rctdl_datapath_resp_t TrcPktProcPtm::onFlush()
{
    rctdl_datapath_resp_t err = RCTDL_RESP_FATAL_NOT_INIT;
    if(m_config != 0)
    {
         err = RCTDL_RESP_CONT;
    }
    return err;
}

const bool TrcPktProcPtm::isBadPacket() const
{
    return m_curr_packet.isBadPacket();
}

void TrcPktProcPtm::InitPacketState()
{
    m_curr_packet.Clear();
}

void TrcPktProcPtm::InitProcessorState()
{
    m_curr_packet.SetType(PTM_PKT_NOTSYNC);
    m_pIPktFn = &TrcPktProcPtm::waitASync;
    m_process_state = PROC_HDR;
    m_curr_packet.ResetState();
    InitPacketState();
}

const bool TrcPktProcPtm::readByte(uint8_t &currByte)
{
    bool bValidByte = false;
    
    if(m_dataInProcessed < m_dataInLen)
    {
        currByte = m_pDataIn[m_dataInProcessed++];
        m_currPacketData.push_back(currByte);
        bValidByte = true;
    }
    return bValidByte;
}

rctdl_datapath_resp_t TrcPktProcPtm::outputPacket()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t TrcPktProcPtm::outputUnsyncData()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}


/*** sync and packet functions ***/
void TrcPktProcPtm::waitASync()
{
}

void TrcPktProcPtm::pktASync()
{
}

void TrcPktProcPtm::pktISync()
{
    uint8_t currByte = 0;
    int pktIndex = m_currPacketData.size() - 1;
    bool bGotBytes = false;

    if(pktIndex == 0)
    {
        m_numCtxtIDBytes = m_config->CtxtIDBytes();
        m_gotCtxtIDBytes = 0;

        // total bytes = 6 + ctxtID; (perhaps more later)
        m_numPktBytesReq = 6 + m_numCtxtIDBytes;
    }

    while(readByte(currByte) && !bGotBytes)
    {
        pktIndex = m_currPacketData.size() - 1;
        if(pktIndex == 5)
        {
            // got the info byte  
            int altISA = (currByte >> 2) & 0x1;
            int reason = (currByte >> 5) & 0x3;
            m_curr_packet.SetISyncReason((ptm_isync_reason_t)(reason));
            m_curr_packet.UpdateNS((currByte >> 3) & 0x1);
            m_curr_packet.UpdateAltISA((currByte >> 2) & 0x1);
            m_curr_packet.UpdateHyp((currByte >> 1) & 0x1);

            rctdl_isa isa = rctdl_isa_arm;
            if(m_currPacketData[1] & 0x1)
                isa = altISA ? rctdl_isa_tee : rctdl_isa_thumb2;
            m_curr_packet.UpdateISA(isa);

            // check cycle count required - not if reason == 0;
            m_needCycleCount = (reason != 0) ? m_config->enaCycleAcc() : false;
            m_gotCycleCount = false;
            m_numPktBytesReq += (m_needCycleCount ? 1 : 0);

        }
        else if(pktIndex > 5)
        {
            if(m_needCycleCount && !m_gotCycleCount)
            {
                if(pktIndex == 6)
                    m_gotCycleCount = (bool)((currByte & 0x40) == 0);   // no cont bit, got cycle count
                else
                    m_gotCycleCount = ((currByte & 0x80) == 0) || (pktIndex == 10);

                if(!m_gotCycleCount)    // need more cycle count bytes
                    m_numPktBytesReq++;
            }
            else if( m_numCtxtIDBytes > m_gotCtxtIDBytes)
            {
                m_gotCtxtIDBytes++;
            }
        }

        // check if we have enough bytes
        bGotBytes = (bool)(m_numPktBytesReq == m_currPacketData.size());
    }

    if(bGotBytes)
    {
        // extract address value, cycle count and ctxt id.
        uint32_t cycleCount = 0;
        uint32_t ctxtID;


    }
}

void TrcPktProcPtm::pktTrigger()
{
}

void TrcPktProcPtm::pktWPointUpdate()
{
}

void TrcPktProcPtm::pktIgnore()
{
}

void TrcPktProcPtm::pktCtxtID()
{
}

void TrcPktProcPtm::pktVMID()
{
}

void TrcPktProcPtm::pktAtom()
{
    uint8_t pHdr = m_currPacketData[0];
    uint32_t cycleCount = 0;
    bool bCycleAccurate = m_config->enaCycleAcc();
    bool bGotAllPktBytes = false;
    uint8_t currByte = 0;

    if(bCycleAccurate)    
    {        
        if(!(pHdr & 0x40))
        {
            // only the header byte present
            cycleCount = (pHdr >> 2) & 0xF;
            bGotAllPktBytes = true;
        }
        else 
        {
            // up to 4 additional bytes of count data.
            while(readByte(currByte) && !bGotAllPktBytes)
            {
                if(!(currByte & 0x80) || (m_currPacketData.size() == 5))
                {
                    bGotAllPktBytes = true;
                    cycleCount = (pHdr >> 2) & 0xF;
                    int shift = 4;
                    uint32_t data;
                    for(int idx = 1; idx < m_currPacketData.size(); idx++)
                    {
                        data = (uint32_t)m_currPacketData[idx];
                        cycleCount |= ((data & 0x7F) << shift);
                        shift += 7;
                    }
                }
            }
        }
    }
    else
        bGotAllPktBytes = true;

    if(bGotAllPktBytes)
    {
        m_curr_packet.UpdateAtomFromPHdr(pHdr,bCycleAccurate,cycleCount);
        m_process_state == SEND_PKT;
    }
}

void TrcPktProcPtm::pktTimeStamp()
{
}

void TrcPktProcPtm::pktExceptionRet()
{
}

void TrcPktProcPtm::pktBranchAddr()
{
}

void TrcPktProcPtm::pktReserved()
{
}


void TrcPktProcPtm::BuildIPacketTable()
{
    // initialise all to branch, atom or reserved packet header
    for(unsigned i = 0; i < 256; i++)
    {
        // branch address packets all end in 8'bxxxxxxx1
        if((i & 0x01) == 0x01)
        {
            m_i_table[i].pkt_type = PTM_PKT_BRANCH_ADDRESS;
            m_i_table[i].pptkFn = &TrcPktProcPtm::pktBranchAddr;
        }
        // atom packets are 8'b1xxxxxx0
        else if((i & 0x81) == 0x80)
        {
            m_i_table[i].pkt_type = PTM_PKT_ATOM;
            m_i_table[i].pptkFn = &TrcPktProcPtm::pktAtom;
        }
        else
        {
            // set all the others to reserved for now
            m_i_table[i].pkt_type = PTM_PKT_RESERVED;
            m_i_table[i].pptkFn = &TrcPktProcPtm::pktReserved;
        }
    }

    // pick out the other packet types by individual codes.

    // A-sync           8'b00000000
    m_i_table[0x00].pkt_type = PTM_PKT_A_SYNC;
    m_i_table[0x00].pptkFn = &TrcPktProcPtm::pktASync;

    // I-sync           8'b00001000
    m_i_table[0x04].pkt_type = PTM_PKT_I_SYNC;
    m_i_table[0x04].pptkFn = &TrcPktProcPtm::pktISync;

    // waypoint update  8'b01110010
    m_i_table[0x72].pkt_type = PTM_PKT_WPOINT_UPDATE;
    m_i_table[0x72].pptkFn = &TrcPktProcPtm::pktWPointUpdate;
    
    // trigger          8'b00001100
    m_i_table[0x0C].pkt_type = PTM_PKT_TRIGGER;
    m_i_table[0x0C].pptkFn = &TrcPktProcPtm::pktTrigger;

    // context ID       8'b01101110
    m_i_table[0x6E].pkt_type = PTM_PKT_CONTEXT_ID;
    m_i_table[0x6E].pptkFn = &TrcPktProcPtm::pktCtxtID;

    // VMID             8'b00111100
    m_i_table[0x3C].pkt_type = PTM_PKT_VMID;
    m_i_table[0x3C].pptkFn = &TrcPktProcPtm::pktVMID;

    // Timestamp        8'b01000x10
    m_i_table[0x42].pkt_type = PTM_PKT_TIMESTAMP;
    m_i_table[0x42].pptkFn = &TrcPktProcPtm::pktTimeStamp;
    m_i_table[0x46].pkt_type = PTM_PKT_TIMESTAMP;
    m_i_table[0x46].pptkFn = &TrcPktProcPtm::pktTimeStamp;

    // Exception return 8'b01110110
    m_i_table[0x76].pkt_type = PTM_PKT_EXCEPTION_RET;
    m_i_table[0x76].pptkFn = &TrcPktProcPtm::pktExceptionRet;

    // Ignore           8'b01100110
    m_i_table[0x66].pkt_type = PTM_PKT_IGNORE;
    m_i_table[0x66].pptkFn = &TrcPktProcPtm::pktIgnore;
}

/* End of File trc_pkt_proc_ptm.cpp */

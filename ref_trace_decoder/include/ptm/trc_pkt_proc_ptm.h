/*
 * \file       trc_pkt_proc_ptm.h
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

#ifndef ARM_TRC_PKT_PROC_PTM_H_INCLUDED
#define ARM_TRC_PKT_PROC_PTM_H_INCLUDED

#include "trc_pkt_types_ptm.h"
#include "trc_pkt_proc_base.h"
#include "trc_pkt_elem_ptm.h"

class PtmTrcPacket;
class PtmConfig;

/** @addtogroup rctdl_pkt_proc
@{*/



class TrcPktProcPtm : public TrcPktProcBase< PtmTrcPacket,  rctdl_ptm_pkt_type, PtmConfig>
{
public:
    TrcPktProcPtm();
    TrcPktProcPtm(int instIDNum);
    virtual ~TrcPktProcPtm();

protected:
    /* implementation packet processing interface */
    virtual rctdl_datapath_resp_t processData(  const rctdl_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed);
    virtual rctdl_datapath_resp_t onEOT();
    virtual rctdl_datapath_resp_t onReset();
    virtual rctdl_datapath_resp_t onFlush();
    virtual rctdl_err_t onProtocolConfig();
    virtual const bool isBadPacket() const;

    void InitPacketState();      // clear current packet state.
    void InitProcessorState();   // clear all previous process state

    rctdl_datapath_resp_t outputPacket();
    rctdl_datapath_resp_t outputUnsyncData();

    typedef enum _process_state {
        PROC_HDR,
        PROC_DATA,
        SEND_PKT, 
    } process_state;
    
    process_state m_process_state;  // process algorithm state.

    std::vector<uint8_t> m_currPacketData;  // raw data
    uint32_t m_currPktIdx;   // index into packet when expanding
    PtmTrcPacket m_curr_packet;  // expanded packet
    rctdl_trc_index_t m_curr_pkt_index; // trace index at start of packet.

    const bool readByte(uint8_t &currByte);

    uint8_t m_chanIDCopy;

    // current data block being processed.
    const uint8_t *m_pDataIn;
    uint32_t m_dataInLen;
    uint32_t m_dataInProcessed;
    rctdl_trc_index_t m_block_idx; // index start for current block

    const bool isSync() const;

    void waitASync();       //!< look for first synchronisation point in the packet stream

    // ** packet processing functions.
    void pktASync();
    void pktISync();
    void pktTrigger();
    void pktWPointUpdate();
    void pktIgnore();
    void pktCtxtID();
    void pktVMID();
    void pktAtom();
    void pktTimeStamp();
    void pktExceptionRet();
    void pktBranchAddr();
    void pktReserved();

    // number of bytes required for a complete packet - used in some multi byte packets
    int m_numPktBytesReq;

    // packet processing state
    bool m_needCycleCount;
    bool m_gotCycleCount;
    int m_numCtxtIDBytes;
    int m_gotCtxtIDBytes;


    // bad packets 
    void throwMalformedPacketErr(const char *pszErrMsg);
    void throwPacketHeaderErr(const char *pszErrMsg);


    // packet processing function table
    typedef void (TrcPktProcPtm::*PPKTFN)(void);
    PPKTFN m_pIPktFn;

    struct _pkt_i_table_t {
        rctdl_ptm_pkt_type pkt_type;
        PPKTFN pptkFn;
    } m_i_table[256];

    void BuildIPacketTable();    

};

inline const bool TrcPktProcPtm::isSync() const
{
    return (bool)(m_curr_packet.getType() == PTM_PKT_NOTSYNC);
}

inline void TrcPktProcPtm::throwMalformedPacketErr(const char *pszErrMsg)
{
    m_curr_packet.SetErrType(PTM_PKT_BAD_SEQUENCE);
    throw rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_BAD_PACKET_SEQ,m_curr_pkt_index,m_chanIDCopy,pszErrMsg);
}

inline void TrcPktProcPtm::throwPacketHeaderErr(const char *pszErrMsg)
{
    throw rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_INVALID_PCKT_HDR,m_curr_pkt_index,m_chanIDCopy,pszErrMsg);
}


/** @}*/

#endif // ARM_TRC_PKT_PROC_PTM_H_INCLUDED

/* End of File trc_pkt_proc_ptm.h */

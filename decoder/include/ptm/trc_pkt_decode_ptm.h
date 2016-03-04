/*
 * \file       trc_pkt_decode_ptm.h
 * \brief      Reference CoreSight Trace Decoder : PTM packet decoder.
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
#ifndef ARM_TRC_PKT_DECODE_PTM_H_INCLUDED
#define ARM_TRC_PKT_DECODE_PTM_H_INCLUDED

#include "trc_pkt_decode_base.h"
#include "ptm/trc_pkt_elem_ptm.h"
#include "ptm/trc_cmp_cfg_ptm.h"
#include "trc_gen_elem.h"


class TrcPktDecodePtm : public TrcPktDecodeBase<PtmTrcPacket, PtmConfig>
{
public:
    TrcPktDecodePtm();
    TrcPktDecodePtm(int instIDNum);
    virtual ~TrcPktDecodePtm();

protected:
    /* implementation packet decoding interface */
    virtual rctdl_datapath_resp_t processPacket();
    virtual rctdl_datapath_resp_t onEOT();
    virtual rctdl_datapath_resp_t onReset();
    virtual rctdl_datapath_resp_t onFlush();
    virtual rctdl_err_t onProtocolConfig();
    virtual const uint8_t getCoreSightTraceID() { return m_CSID; };

    /* local decode methods */

private:
    void initDecoder();
    void resetDecoder();

    rctdl_datapath_resp_t decodePacket();
    rctdl_datapath_resp_t processIsync();
    rctdl_datapath_resp_t processBranch();
    rctdl_datapath_resp_t processWPUpdate();
    rctdl_datapath_resp_t processAtom();
    rctdl_err_t traceInstrToWP(bool &bWPFound);      //!< follow instructions from the current address to a WP. true if good, false if memory cannot be accessed.

    uint8_t m_CSID; //!< Coresight trace ID for this decoder.

//** Other processor state;

    // trace decode FSM
    typedef enum {
        NO_SYNC,        //!< pre start trace - init state or after reset or overflow, loss of sync.
        WAIT_SYNC,      //!< waiting for sync packet.
        WAIT_ISYNC,     //!< waiting for isync packet after 1st ASYNC.
        DECODE_PKTS,    //!< processing packets 
        OUTPUT_PKT,     //!< need to output any available packet.
    } processor_state_t;

    processor_state_t m_curr_state;

    typedef struct _ptm_decode_state {
            rctdl_isa isa;
            rctdl_vaddr_t instr_addr; 
            bool valid;
    } ptm_decode_state;


    // packet decode state
    bool m_need_isync;   //!< need context to continue
    bool m_need_addr;   //!< need an address to continue
    bool m_except_pending_addr;    //!< next address packet is part of exception.
    
    rctdl_instr_info m_instr_info;  //!< instruction info for code follower - in address is the next to be decoded.

    bool m_mem_nacc_pending;    //!< need to output a memory access failure packet
    rctdl_vaddr_t m_nacc_addr;  //!< 
    bool m_is_secure;           //!< current secure state

    rctdl_pe_context m_pe_context;  //!< current context information
    ptm_decode_state m_current_state;   //!< current instruction state for PTM decode.
    ptm_decode_state m_last_state;      //!< last instruction state for PTM decode.

    bool m_b_part_isync;        //!< isync processing can generate multiple generic packets.
    bool m_b_i_sync_pe_context; //1< isync needs pe context.

//** output element
    RctdlTraceElement m_output_elem;
};

#endif // ARM_TRC_PKT_DECODE_PTM_H_INCLUDED

/* End of File trc_pkt_decode_ptm.h */

/*!
 * \file       trc_pkt_decode_etmv3.h
 * \brief      OpenCSD : ETMv3 decode
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

#ifndef ARM_TRC_PKT_DECODE_ETMV3_H_INCLUDED
#define ARM_TRC_PKT_DECODE_ETMV3_H_INCLUDED

#include "common/trc_pkt_decode_base.h"
#include "common/trc_gen_elem.h"
#include "common/ocsd_pe_context.h"
#include "common/ocsd_code_follower.h"

#include "etmv3/trc_pkt_elem_etmv3.h"
#include "etmv3/trc_cmp_cfg_etmv3.h"


class TrcPktDecodeEtmV3 :  public TrcPktDecodeBase<EtmV3TrcPacket, EtmV3Config>
{
public:
    TrcPktDecodeEtmV3();
    TrcPktDecodeEtmV3(int instIDNum);
    virtual ~TrcPktDecodeEtmV3();

protected:
    /* implementation packet decoding interface */
    virtual ocsd_datapath_resp_t processPacket();
    virtual ocsd_datapath_resp_t onEOT();
    virtual ocsd_datapath_resp_t onReset();
    virtual ocsd_datapath_resp_t onFlush();
    virtual ocsd_err_t onProtocolConfig();
    virtual const uint8_t getCoreSightTraceID() { return m_CSID; };

    /* local decode methods */
    void initDecoder();      //!< initial state on creation (zeros all config)
    void resetDecoder();     //!< reset state to start of decode. (moves state, retains config)

    ocsd_datapath_resp_t decodePacket(); //!< decode a packet

    ocsd_datapath_resp_t processISync(const bool withCC);
    ocsd_datapath_resp_t processBranchAddr();
    ocsd_datapath_resp_t processPHdr();
    
    void pendPacket();  //!< save current packet for re-assess next pass

private:
    
//** intra packet state;

    OcsdCodeFollower m_code_follower;   //!< code follower for instruction trace

    ocsd_vaddr_t m_IAddr;           //!< next instruction address
    OcsdPeContext m_pe_context;     //!< context for the PE

    EtmV3TrcPacket m_pended_packet; //! Saved packet when processing pended.

//** Other packet decoder state;

    // trace decode FSM
    typedef enum {
        NO_SYNC,        //!< pre start trace - init state or after reset or overflow, loss of sync.
        WAIT_ASYNC,     //!< waiting for a-sync packet.
        WAIT_ISYNC,     //!< waiting for i-sync packet.
        DECODE_PKTS,    //!< processing a packet
        PEND_INSTR,     //!< instruction output pended - need to check next packet for cancel - data in output element
        PEND_PACKET,    //!< packet decode pended - save previous none decoded packet data - prev pended instr had wait response.
    } processor_state_t;

    processor_state_t m_curr_state;

    uint8_t m_CSID; //!< Coresight trace ID for this decoder.

//** output element
    OcsdTraceElement m_output_elem;
};


#endif // ARM_TRC_PKT_DECODE_ETMV3_H_INCLUDED

/* End of File trc_pkt_decode_etmv3.h */

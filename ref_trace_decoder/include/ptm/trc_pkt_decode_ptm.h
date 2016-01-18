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


    uint8_t m_CSID; //!< Coresight trace ID for this decoder.

//** output element
    RctdlTraceElement m_output_elem;
};

#endif // ARM_TRC_PKT_DECODE_PTM_H_INCLUDED

/* End of File trc_pkt_decode_ptm.h */

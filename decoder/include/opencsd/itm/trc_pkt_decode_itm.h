/*
 * \file       trc_pkt_decode_itm.h
 * \brief      OpenCSD : ITM packet decoder
 *
 *  Convert the incoming indidvidual ITM packets to generic output packets.
 *
 * \copyright  Copyright (c) 2024, ARM Limited. All Rights Reserved.
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


#ifndef ARM_TRC_PKT_DECODE_ITM_H_INCLUDED
#define ARM_TRC_PKT_DECODE_ITM_H_INCLUDED


#include "common/trc_pkt_decode_base.h"
#include "opencsd/itm/trc_pkt_elem_itm.h"
#include "opencsd/itm/trc_cmp_cfg_itm.h"
#include "common/trc_gen_elem.h"

class TrcPktDecodeItm : public TrcPktDecodeBase<ItmTrcPacket, ITMConfig>
{
public:
    TrcPktDecodeItm();
    TrcPktDecodeItm(int instIDNum);
    virtual ~TrcPktDecodeItm();

protected:
    /* implementation packet decoding interface */
    virtual ocsd_datapath_resp_t processPacket();
    virtual ocsd_datapath_resp_t onEOT();
    virtual ocsd_datapath_resp_t onReset();
    virtual ocsd_datapath_resp_t onFlush();
    virtual ocsd_err_t onProtocolConfig();
    virtual const uint8_t getCoreSightTraceID() { return m_CSID; };

private:
    void initDecoder();
    void resetDecoder();
    ocsd_datapath_resp_t decodePacket();  //!< decode the current incoming packet
    void initItmInfo();

    typedef enum {
        NO_SYNC,        //!< pre start trace - init state or after reset or overflow, loss of sync.
        WAIT_SYNC,      //!< waiting for sync packet.
        DECODE_PKTS     //!< processing input packet.  
    } processor_state_t;

    processor_state_t m_curr_state;
    unsync_info_t m_unsync_info;
    swt_itm_info m_itm_info;    // output info for itm
        
    uint8_t m_CSID;
    
    uint64_t m_LocalTSCount;  //!< Aggregate count for local timestamps - multiplied by any prescaler in operation
    uint64_t m_GlobalTS;      //!< Current global timestamp count
    uint8_t m_StimPage;       //!< current page value for stimulation write ID channel
    bool m_b_needGTS2;        //!< waiting for GTS2 to output a global timestamp
    bool m_b_GTSFreqChange;   //!< indication the GTS has changed frequency.
    bool m_b_prevOverflow;    //!< previous packet was an overflow - mark current packet

    //** output element
    OcsdTraceElement m_output_elem; //!< output packet
};


inline void TrcPktDecodeItm::initItmInfo()
{
    m_itm_info.overflow = 0;
    m_itm_info.payload_size = 0;
    m_itm_info.payload_src_id = 0;
    m_itm_info.value = 0;
}

#endif // ARM_TRC_PKT_DECODE_ITM_H_INCLUDED

/* End of File trc_pkt_decode_itm.h */

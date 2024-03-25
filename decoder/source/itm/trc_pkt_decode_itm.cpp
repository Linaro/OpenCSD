/*
 * \file       trc_pkt_decode_itm.cpp
 * \brief      OpenCSD : ITM packet decoder - output generic ITM-SW trace packets.
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

#include "opencsd/itm/trc_pkt_decode_itm.h"
#define DCD_NAME "DCD_ITM"

TrcPktDecodeItm::TrcPktDecodeItm()
    : TrcPktDecodeBase(DCD_NAME)
{
    initDecoder();
}

TrcPktDecodeItm::TrcPktDecodeItm(int instIDNum)
    : TrcPktDecodeBase(DCD_NAME, instIDNum)
{
    initDecoder();
}

TrcPktDecodeItm::~TrcPktDecodeItm()
{
}

/* implementation packet decoding interface */
ocsd_datapath_resp_t TrcPktDecodeItm::processPacket()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    bool bPktDone = false;

    while (!bPktDone)
    {
        switch (m_curr_state)
        {
        case NO_SYNC:
            m_output_elem.setType(OCSD_GEN_TRC_ELEM_NO_SYNC);
            m_output_elem.setUnSyncEOTReason(m_unsync_info);
            resp = outputTraceElement(m_output_elem);
            m_curr_state = WAIT_SYNC;
            break;

        case WAIT_SYNC:
            if (m_curr_packet_in->getPktType() == ITM_PKT_ASYNC)
                m_curr_state = DECODE_PKTS;
            bPktDone = true;
            break;

        case DECODE_PKTS:
            resp = decodePacket();
            bPktDone = true;
            break;
        }
    }
    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeItm::onEOT()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    m_output_elem.setType(OCSD_GEN_TRC_ELEM_EO_TRACE);
    m_output_elem.setUnSyncEOTReason(UNSYNC_EOT);
    resp = outputTraceElement(m_output_elem);
    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeItm::onReset()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    m_unsync_info = UNSYNC_RESET_DECODER;
    // resetDecoder();
    return resp;
}

ocsd_datapath_resp_t TrcPktDecodeItm::onFlush()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    // don't currently save unsent packets so nothing to flush
    return resp;
}

ocsd_err_t TrcPktDecodeItm::onProtocolConfig()
{
    if (m_config == 0)
        return OCSD_ERR_NOT_INIT;

    // static config - copy of CSID for easy reference
    m_CSID = m_config->getTraceID();
    return OCSD_OK;
}

/***** decode helper functions *********/
void TrcPktDecodeItm::initDecoder()
{
    m_CSID = 0;

    // base decoder state - ITM requires no memory and instruction decode.
    setUsesMemAccess(false);
    setUsesIDecode(false);
    m_unsync_info = UNSYNC_INIT_DECODER;
    resetDecoder();
}

void TrcPktDecodeItm::resetDecoder()
{
    m_output_elem.init();
    m_curr_state = NO_SYNC;

    m_LocalTSCount = 0;     //!< Aggregate count for local timestamps
    m_GlobalTS = 0;         //!< Current global timestamp count
    m_StimPage = 0;         //!< current page value for stimulation write ID channel
    m_b_needGTS2 = true;    //!< waiting for GTS2 to output a global timestamp
    m_b_prevOverflow = false;
    m_b_GTSFreqChange = false;
}

ocsd_datapath_resp_t TrcPktDecodeItm::decodePacket()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    const uint64_t globalTSLowMask[] = {
        0x00000007F, // [ 6:0] 
        0x000003FFF, // [13:0] 
        0x0001FFFFF, // [20:0] 
        0x003FFFFFF, // [25:0]
    };   
    const uint64_t globalTSHiMask = ~globalTSLowMask[3]; // rest of bits (48 /64 impl dep)
    bool bSendPacket = false;       // flag to indicate output required.
    uint8_t src_id = m_curr_packet_in->getSrcID();
        
    static swt_itm_type localTS_TC_types[] = {
        TS_SYNC,            /**< Local TS Synchronous with previous SWIT payload */
        TS_DELAY,           /**< Local TS Delayed from previous SWIT payload */
        TS_PKT_DELAY,       /**< Local TS with previous SWIT payload delayed */
        TS_PKT_TS_DELAY,
    };

    initItmInfo();

    switch (m_curr_packet_in->getPktType())
    {
    default:
    case ITM_PKT_BAD_SEQUENCE:
    case ITM_PKT_RESERVED:
        resp = OCSD_RESP_FATAL_INVALID_DATA;
        m_unsync_info = UNSYNC_BAD_PACKET;
    case ITM_PKT_NOTSYNC:
        resetDecoder();
        break;

    case ITM_PKT_ASYNC:
    case ITM_PKT_INCOMPLETE_EOT:
        break;

    case ITM_PKT_DWT:
        // output payload and discriminator
        m_itm_info.pkt_type = DWT_PAYLOAD;
        m_itm_info.payload_size = m_curr_packet_in->getValSize();
        m_itm_info.value = m_curr_packet_in->getValue();
        m_itm_info.payload_src_id = src_id;
        bSendPacket = true;
        break;

    case ITM_PKT_SWIT:
        // output payload and channel 
        m_itm_info.pkt_type = SWIT_PAYLOAD;
        m_itm_info.payload_size = m_curr_packet_in->getValSize();
        m_itm_info.value = m_curr_packet_in->getValue();
        src_id = (src_id & 0x1F) | (m_StimPage << 5);
        m_itm_info.payload_src_id = src_id;
        bSendPacket = true;
        break;

    case ITM_PKT_EXTENSION:
        // only process extension packet that we understand - page for channel ID
        // SW/HW bit = 1b0, N size = 2
        if (((src_id & 0x80) == 0) && ((src_id & 0x1f) == 2))
            m_StimPage = (uint8_t)m_curr_packet_in->getValue();
        break;

    case ITM_PKT_OVERFLOW:
        // overflow - mark for next packet - reset accumulated local TS
        m_LocalTSCount = 0;
        m_b_prevOverflow = true;
        break;

    case ITM_PKT_TS_GLOBAL_1:        
        // if this is marked as wrapped then wait for next GTS2 - also save freq change flag  
        if (!m_b_needGTS2)
            m_b_needGTS2 = (bool)(src_id & 0x2);
        if (!m_b_GTSFreqChange)
            m_b_GTSFreqChange = (bool)(src_id & 0x1);

        // update lower bits - but only send if not waiting for GTS2
        m_GlobalTS &= ~globalTSLowMask[m_curr_packet_in->getValSize() - 1];
        m_GlobalTS |= (uint64_t)(m_curr_packet_in->getValue());

        if (!m_b_needGTS2) {
            m_itm_info.pkt_type = TS_GLOBAL;                  
            m_output_elem.setTS(m_GlobalTS, m_b_GTSFreqChange);
            m_b_GTSFreqChange = false;
            bSendPacket = true;
        }
        break;

    case ITM_PKT_TS_GLOBAL_2:
        // set top bits - send TS packet
        m_itm_info.pkt_type = TS_GLOBAL;
        m_GlobalTS &= ~globalTSHiMask; // clear top bits
        m_GlobalTS |= ((m_curr_packet_in->getExtValue()) << 26);
        m_output_elem.setTS(m_GlobalTS, m_b_GTSFreqChange);
        m_b_GTSFreqChange = false;
        m_b_needGTS2 = false;
        bSendPacket = true;
        break;

    case ITM_PKT_TS_LOCAL:
        // send local TS packet
        // add to aggregate and set in TS.
        m_itm_info.pkt_type = localTS_TC_types[src_id & 0x3];
        m_itm_info.payload_size = m_curr_packet_in->getValSize();
        m_itm_info.value = m_curr_packet_in->getValue();
        m_LocalTSCount += ((uint64_t)m_itm_info.value) * m_config->getTSPrescaleValue();
        m_output_elem.setTS(m_LocalTSCount);
        bSendPacket = true;
        break;

    }

    if (bSendPacket)
    {
        if (m_b_prevOverflow) 
        {
            m_itm_info.overflow = 1;
            m_b_prevOverflow = false;
        }
        m_output_elem.setType(OCSD_GEN_TRC_ELEM_ITMTRACE);
        m_output_elem.setSWT_ITMInfo(m_itm_info);
        resp = outputTraceElement(m_output_elem);
    }

    return resp;
}
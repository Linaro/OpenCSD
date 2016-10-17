/*
 * \file       trc_pkt_decode_stm.cpp
 * \brief      OpenCSD : 
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

#include "stm/trc_pkt_decode_stm.h"
#define DCD_NAME "DCD_STM"

TrcPktDecodeStm::TrcPktDecodeStm()
    : TrcPktDecodeBase(DCD_NAME)
{
    initDecoder();
}

TrcPktDecodeStm::TrcPktDecodeStm(int instIDNum)
    : TrcPktDecodeBase(DCD_NAME, instIDNum)
{
    initDecoder();
}

TrcPktDecodeStm::~TrcPktDecodeStm()
{
    if(m_payload_buffer)
        delete [] m_payload_buffer;
    m_payload_buffer = 0;
}

/* implementation packet decoding interface */
ocsd_datapath_resp_t TrcPktDecodeStm::processPacket()
{
}

ocsd_datapath_resp_t TrcPktDecodeStm::onEOT()
{
}

ocsd_datapath_resp_t TrcPktDecodeStm::onReset()
{
}

ocsd_datapath_resp_t TrcPktDecodeStm::onFlush()
{
}

ocsd_err_t TrcPktDecodeStm::onProtocolConfig()
{
    if(m_config == 0)
        return OCSD_ERR_NOT_INIT;

    // static config - copy of CSID for easy reference
    m_CSID = m_config->getTraceID();
    return OCSD_OK;
}

void TrcPktDecodeStm::initDecoder()
{
    m_payload_buffer = 0;
    m_payload_used = 0; 
    resetDecoder();
}

void TrcPktDecodeStm::resetDecoder()
{
    m_curr_state = NO_SYNC;
    m_curr_master = 0;
    m_curr_channel = 0;
    m_payload_size = 0;     
    m_payload_odd_nibble = false;
    m_CSID = 0;
    m_output_elem.init();
}

void TrcPktDecodeStm::initPayloadBuffer()
{
    // set up the payload buffer. If we are correlating indentical packets then 
    // need a buffer that is a multiple of 64bit packets.
    // otherwise a single packet length will do.


}

/* End of File trc_pkt_decode_stm.cpp */

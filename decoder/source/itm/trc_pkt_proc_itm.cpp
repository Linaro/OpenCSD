/*
 * \file       trc_pkt_proc_itm.cpp
 * \brief      OpenCSD :
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

#include "opencsd/itm/trc_pkt_proc_itm.h"

  // processor object construction
  // ************************

#ifdef __GNUC__
// G++ doesn't like the ## pasting
#define ITM_PKTS_NAME "PKTP_ITM"
#else
#define ITM_PKTS_NAME OCSD_CMPNAME_PREFIX_PKTPROC##"_ITM"
#endif

static const uint32_t ITM_SUPPORTED_OP_FLAGS = OCSD_OPFLG_PKTPROC_COMMON;

TrcPktProcItm::TrcPktProcItm() : TrcPktProcBase(ITM_PKTS_NAME)
{
    initObj();
}

TrcPktProcItm::TrcPktProcItm(int instIDNum) : TrcPktProcBase(ITM_PKTS_NAME, instIDNum)
{
    initObj();
}

TrcPktProcItm::~TrcPktProcItm()
{
    getRawPacketMonAttachPt()->set_notifier(0);
}

void TrcPktProcItm::initObj()
{
    m_supported_op_flags = ITM_SUPPORTED_OP_FLAGS;
    initProcessorState();
}

void TrcPktProcItm::initProcessorState()
{
    // clear any state that persists between packets
    setProcUnsynced();
    initNextPacket();
    m_sent_notsync_packet = false;
    m_sync_start = false;
    m_dump_unsynced_bytes = 0;
}

void TrcPktProcItm::initNextPacket()
{
    // clear state that is unique to each packet
    m_packet_data.clear();
    m_curr_packet.initPacket();
}

// implementation packet processing interface overrides 
// ************************

ocsd_datapath_resp_t TrcPktProcItm::processData(const ocsd_trc_index_t index,
    const uint32_t dataBlockSize,
    const uint8_t* pDataBlock,
    uint32_t* numBytesProcessed)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    m_p_data_in = pDataBlock;
    m_data_in_size = dataBlockSize;
    m_data_in_used = 0;

    // while there is data and a continue response on the data path
    while (dataToProcess() && OCSD_DATA_RESP_IS_CONT(resp))
    {
        try
        {
            switch (m_proc_state)
            {
            case WAIT_SYNC:
                resp = waitForSync(index);
                break;

            case PROC_HDR:
                m_packet_index = index + m_data_in_used;
                itmProcessHdr(); // will set to PROC_DATA or SEND_PKT on valid header.

                // if we are not now in data processing, break, otherwise fall through
                if (m_proc_state != PROC_DATA)
                    break;

            case PROC_DATA:
                (this->*m_pCurrPktFn)();

                // if we have enough to send, fall through, otherwise stop
                if (m_proc_state != SEND_PKT)
                    break;

            case SEND_PKT:
                resp = outputPacket();
                break;
            }
        }
        catch (ocsdError& err)
        {
            LogError(err);
            if (((err.getErrorCode() == OCSD_ERR_BAD_PACKET_SEQ) ||
                (err.getErrorCode() == OCSD_ERR_INVALID_PCKT_HDR)) &&
                !(getComponentOpMode() & OCSD_OPFLG_PKTPROC_ERR_BAD_PKTS))
            {
                // send invalid packets up the pipe to let the next stage decide what to do.
                resp = outputPacket();
                if (getComponentOpMode() & OCSD_OPFLG_PKTPROC_UNSYNC_ON_BAD_PKTS)
                    m_proc_state = WAIT_SYNC;
            }
            else
            {
                // bail out on any other error.
                resp = OCSD_RESP_FATAL_INVALID_DATA;
            }
        }
        catch (...)
        {
            /// vv bad at this point.
            resp = OCSD_RESP_FATAL_SYS_ERR;
            ocsdError fatal = ocsdError(OCSD_ERR_SEV_ERROR, OCSD_ERR_FAIL, m_packet_index, m_config->getTraceID());
            fatal.setMessage("Unknown System Error decoding trace.");
            LogError(fatal);
        }
    }

    *numBytesProcessed = m_data_in_used;
    return resp;
}

ocsd_datapath_resp_t TrcPktProcItm::onEOT()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    if (m_proc_state == PROC_DATA)   // there is a partial packet in flight
    {
        m_curr_packet.updateErrType(ITM_PKT_INCOMPLETE_EOT);    // re mark as incomplete
        resp = outputPacket();
    }
    return resp;
}

ocsd_datapath_resp_t TrcPktProcItm::onReset()
{
    initProcessorState();
    return OCSD_RESP_CONT;
}

ocsd_datapath_resp_t TrcPktProcItm::onFlush()
{
    // packet processor never holds on to flushable data (may have partial packet, 
    // but any full packets are immediately sent)
    return OCSD_RESP_CONT;
}

ocsd_err_t TrcPktProcItm::onProtocolConfig()
{
    return OCSD_OK;  // nothing to do on config for this processor
}

const bool TrcPktProcItm::isBadPacket() const
{
    return m_curr_packet.isBadPacket();
}

ocsd_datapath_resp_t TrcPktProcItm::outputPacket()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    resp = outputOnAllInterfaces(m_packet_index, &m_curr_packet, &m_curr_packet.type, m_packet_data);
    m_packet_data.clear();
    initNextPacket();
    m_proc_state = m_bStreamSync ? PROC_HDR : WAIT_SYNC;
    return resp;
}

void TrcPktProcItm::throwBadSequenceError(const char* pszMessage /*= ""*/)
{
    m_curr_packet.updateErrType(ITM_PKT_BAD_SEQUENCE);
    throw ocsdError(OCSD_ERR_SEV_ERROR, OCSD_ERR_BAD_PACKET_SEQ, m_packet_index, this->m_config->getTraceID(), pszMessage);
}

void TrcPktProcItm::throwReservedHdrError(const char* pszMessage /*= ""*/)
{
    m_curr_packet.setPktType(ITM_PKT_RESERVED);
    throw ocsdError(OCSD_ERR_SEV_ERROR, OCSD_ERR_INVALID_PCKT_HDR, m_packet_index, this->m_config->getTraceID(), pszMessage);
}

/**************/
/* packet processing routines */
const bool TrcPktProcItm::readByte(uint8_t &byte)
{
    bool gotData = true;

    if (m_data_in_used < m_data_in_size)
    {
        byte = m_p_data_in[m_data_in_used++];
        savePacketByte(byte);
    }
    else
        gotData = false;
    return gotData;
}

void TrcPktProcItm::itmProcessHdr()
{    
    // no data, just return and fall out of processing routine.
    if (!readByte(m_header_byte))
        return;

    if ((m_header_byte & 0x03) != 0x00)
    {
        // Stimulus data packets
        if (m_header_byte & 0x4)
            m_curr_packet.setPktType(ITM_PKT_DWT);
        else
            m_curr_packet.setPktType(ITM_PKT_SWIT);
        m_pCurrPktFn = &TrcPktProcItm::itmPktData;
        m_proc_state = PROC_DATA;
    }
    else if ((m_header_byte & 0x0F) == 0x00)
    {
        // bottom 4 bits 0b0000 - ASYNC, OVERFLOW or Local TS
        if ((m_header_byte & 0xF0) == 0x00)
        {
            // ASYNC
            m_curr_packet.setPktType(ITM_PKT_ASYNC);
            m_pCurrPktFn = &TrcPktProcItm::itmPktAsync;
            m_proc_state = PROC_DATA;
        }
        else if ((m_header_byte & 0xF0) == 0x70)
        {
            // Overflow
            m_curr_packet.setPktType(ITM_PKT_OVERFLOW);
            m_proc_state = SEND_PKT;
        }
        else
        {
            // LOCAL TS
            m_curr_packet.setPktType(ITM_PKT_TS_LOCAL);
            m_pCurrPktFn = &TrcPktProcItm::itmPktLocalTS;
            m_proc_state = PROC_DATA;
        }
    }
    else if ((m_header_byte & 0x0B) == 0x08)
    {
        // looking for extension packet 0b xxxx 1x00
        m_curr_packet.setPktType(ITM_PKT_EXTENSION);
        m_pCurrPktFn = &TrcPktProcItm::itmPktExtension;
        m_proc_state = PROC_DATA;

    }
    else if ((m_header_byte & 0xDF) == 0x94)
    {
        // looking for global ts packet 0b 10x1 0100 -> mask = 8b 1101 1111
        if ((m_header_byte & 0x20) == 0x00)
        {
            m_curr_packet.setPktType(ITM_PKT_TS_GLOBAL_1);
            m_pCurrPktFn = &TrcPktProcItm::itmPktGlobalTS1;
        }
        else
        {
            m_curr_packet.setPktType(ITM_PKT_TS_GLOBAL_2);
            m_pCurrPktFn = &TrcPktProcItm::itmPktGlobalTS2;
        }
        m_proc_state = PROC_DATA;
    }
    else
        throwReservedHdrError();
}

void TrcPktProcItm::itmPktData()
{
    uint8_t byte;
    uint32_t value;
    int payload_bytes_req = m_header_byte & 0x3;
    int payload_bytes_got = (int)(m_packet_data.size() - 1);

    if (payload_bytes_req == 3)
        payload_bytes_req = 4;

    // got a SWIT or DWT header - process data according to size
    if (m_packet_data.size() == 1)
    {
        // save the stimulus address / HW src descriminator - 5 bits 
        m_curr_packet.setSrcID((m_header_byte >> 3) & 0x1F);
    }

    // loop to get payload bytes
    while (payload_bytes_got < payload_bytes_req)
    {
        // readbyte will put data into the array
        if (!readByte(byte))
            break;
        payload_bytes_got++;
    }

    if (payload_bytes_got == payload_bytes_req)
    {
        value = (uint32_t)m_packet_data[1];
        if (payload_bytes_req >= 2)
            value |= (((uint32_t)m_packet_data[2]) << 8);
        if (payload_bytes_req == 4)
        {
            value |= (((uint32_t)m_packet_data[3]) << 16);
            value |= (((uint32_t)m_packet_data[4]) << 24);
        }
        m_curr_packet.setValue(value, payload_bytes_req);
        m_proc_state = SEND_PKT;
    }
}

// read bytes with continue bit into buffer. Return true if last byte seen, false if out of data.
// limit is set to max bytes in packet - stop overrun on data errors
bool TrcPktProcItm::readContBytes(uint32_t limit)
{
    bool bDone = false;
    uint8_t byte;
    
    while (!bDone && (m_packet_data.size() < limit))
    {
        if (!readByte(byte))
            break;
        bDone = ((byte & 0x80) == 0x00);
    }
    return bDone;
}

uint32_t TrcPktProcItm::extractContVal32()
{
    uint32_t value = 0;
    int shift = 0;
    int idx, idx_max = (int)m_packet_data.size() - 1;

    for (idx = 1, shift = 0; idx <= idx_max; idx++, shift += 7)
        value |= (((uint32_t)m_packet_data[idx] & 0x7F) << shift);

    return value;
}

uint64_t TrcPktProcItm::extractContVal64()
{
    uint64_t value = 0;
    int shift = 0;
    int idx, idx_max = (int)m_packet_data.size() - 1;

    for (idx = 1, shift = 0; idx <= idx_max; idx++, shift += 7)
        value |= (((uint64_t)m_packet_data[idx] & 0x7F) << shift);

    return value;
}


void TrcPktProcItm::itmPktLocalTS()
{
    bool bGotContVal = false;
    const int pkt_size_limit = 5; // header + up to 4 payload bytes

    if (m_packet_data.size() == 1)
    {
        // set the TC value into the src_id parameter - if cont bit set
        if (m_header_byte & 0x80)
            m_curr_packet.setSrcID((m_header_byte >> 4) & 0x3);
        else
        {
            // otherwise a single byte Local TS - value in header, TS_SYNC (TC = 2b00) packet
            m_curr_packet.setSrcID(0);
            m_curr_packet.setValue((m_header_byte >> 4) & 0x7, 1);
            m_proc_state = SEND_PKT;
            return;
        }
    }

    bGotContVal = readContBytes(pkt_size_limit);

    if (bGotContVal)
    {
        m_curr_packet.setValue(extractContVal32(), (int)m_packet_data.size() - 1);
        m_proc_state = SEND_PKT;
    }
    else if (m_packet_data.size() == pkt_size_limit)
    {
        throwBadSequenceError("Local TS packet: Payload continuation value too long");
    }
}

void TrcPktProcItm::itmPktGlobalTS1()
{
    bool bGotContVal = false;
    const int pkt_size_limit = 5; // header + up to 4 payload bytes
    uint8_t byte;

    // nothing to process from the header - just do cont payload.
    bGotContVal = readContBytes(pkt_size_limit);

    if (bGotContVal)
    {
        // if we have a fourth payload byte - this contains additional data.
        if (m_packet_data.size() == 5)
        {
            byte = m_packet_data[4];
            // bits [6:5] are wrap and clk change - move these to src_id
            m_curr_packet.setSrcID(((byte >> 5) & 0x3));
            // clear those bits from the data before extracting thevalue
            m_packet_data[4] = byte & 0x1F;
        }
        m_curr_packet.setValue(extractContVal32(), (int)m_packet_data.size() - 1);
        m_proc_state = SEND_PKT;
    }
    else if (m_packet_data.size() == pkt_size_limit)
    {
        throwBadSequenceError("GTS1 packet: Payload continuation value too long");
    }
}

void TrcPktProcItm::itmPktGlobalTS2()
{
    bool bGotContVal = false;
    const int pkt_size_limit = 7; // header + up to 6 payload bytes

    // nothing to process from the header - just do cont payload.
    bGotContVal = readContBytes(pkt_size_limit);

    if (bGotContVal)
    {
        if (m_packet_data.size() <= 5)
            m_curr_packet.setValue(extractContVal32(), (int)m_packet_data.size() - 1);
        else
            m_curr_packet.setExtValue(extractContVal64());
        m_proc_state = SEND_PKT;
    }
    else if (m_packet_data.size() == pkt_size_limit)
    {
        throwBadSequenceError("GTS2 packet: Payload continuation value too long");
    }
}

void TrcPktProcItm::itmPktExtension()
{
    bool bGotContVal = false;
    const int pkt_size_limit = 5; // header + up to 4 payload bytes
    const uint8_t N_bit_length[] = { 2, 9, 16, 23, 31 }; // bitlengths for payload.
    uint8_t src_id_val = 0;
    uint32_t value = 0;

    // we can have just the header..
    if ((m_header_byte & 0x80) == 0)
        bGotContVal = true;
    else
        bGotContVal = readContBytes(pkt_size_limit);

    if (bGotContVal)
    {
        // put bit length and information source bit into src_id
        // [7] = SH (software - 0b0 / hardware - 0b1) source
        // [4:0] = max N bit index of payload.
        src_id_val = N_bit_length[m_packet_data.size() - 1];
        if (m_header_byte & 0x4)
            src_id_val |= 0x80;
        m_curr_packet.setSrcID(src_id_val);

        // now set the value
        if (m_packet_data.size() > 1) 
        {
            value = extractContVal32();
            value <<= 3;
        }
        value |= (uint32_t)((m_header_byte >> 4) & 0x7);
        m_curr_packet.setValue(value, 4);
        m_proc_state = SEND_PKT;
    }
    else if (m_packet_data.size() == pkt_size_limit)
    {
        throwBadSequenceError("Extension packet: Payload continuation value too long");
    }
}

bool TrcPktProcItm::readAsyncSeq(bool &bError)
{
    uint8_t byte;
    bool bFoundAsync = false;

    bError = false;

    // at least 5 0x00 packets - non zero an error
    while ((m_packet_data.size() < 5) && !bError)
    {
        if (!readByte(byte))
            break;

        if (byte != 0x00)
            bError = true;
    }

    // now can be more 0x00 before a 0x80 value - other non zero an error.
    while (!bFoundAsync && !bError)
    {
        if (!readByte(byte))
            break;

        if (byte == 0x80)
            bFoundAsync = true;
        else if (byte != 0x00)
            bError = true;
    }

    return bFoundAsync;
}

// processing an async packet when synchronised - see waitForSync for processing data to search for first async.
void TrcPktProcItm::itmPktAsync()
{
    bool bFoundAsync = false, bError = false;

    bFoundAsync = readAsyncSeq(bError);

    if (bFoundAsync)
        m_proc_state = SEND_PKT;
    else if (bError)
        throwBadSequenceError("Async Packet: unexpected none zero value");
}


ocsd_datapath_resp_t TrcPktProcItm::waitForSync(const ocsd_trc_index_t blk_st_index)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    uint8_t byte;
    bool bAsyncErr;

    m_curr_packet.setPktType(ITM_PKT_NOTSYNC);

    m_dump_unsynced_bytes = 0;

    // not currently processing a possible async sequence so set index to start of current input data.
    if (!m_sync_start)
        m_packet_index = blk_st_index + m_data_in_used;

    while (!m_bStreamSync && dataToProcess() && OCSD_DATA_RESP_IS_CONT(resp))
    {
        if (m_sync_start)
        {
            // continue look for sync pattern in data
            m_bStreamSync = readAsyncSeq(bAsyncErr);
            if (m_bStreamSync)
            {
                m_curr_packet.setPktType(ITM_PKT_ASYNC);
                m_proc_state = SEND_PKT;
            }
            else if (bAsyncErr)
            {
                // not found as not async pattern - back to hunt for start of async packet
                m_dump_unsynced_bytes = (int)m_packet_data.size();
                m_sync_start = false;
            }
        }

        if (!m_sync_start)
        {

            if (!readByte(byte))
                break;

            if (byte == 0x00) 
            {
                // potential async header
                m_sync_start = true;

                // get rid of any preceding bytes
                resp = flushUnsyncedBytes();

                // set index to the possible async header
                m_packet_index = blk_st_index + m_data_in_used - 1;
            }
            else
            {
                m_dump_unsynced_bytes++;

                // periodically flush unsynced bytes during search.
                if (m_dump_unsynced_bytes >= 8)
                    resp = flushUnsyncedBytes();
            }
            
        }        
    }

    // not found sync and run out of data
    if (!m_bStreamSync && !m_sync_start)
        resp = flushUnsyncedBytes();

    return resp;
}

ocsd_datapath_resp_t TrcPktProcItm::flushUnsyncedBytes()
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    // we need to dump unsynced bytes to raw monitor if in use, and send a not sync packet if this is the first time.
    outputRawPacketToMonitor(m_packet_index, &m_curr_packet, m_dump_unsynced_bytes, &m_packet_data[0]);

    if (!m_sent_notsync_packet)
    {
        resp = outputDecodedPacket(m_packet_index, &m_curr_packet);
        m_sent_notsync_packet = true;
    }

    if (m_packet_data.size() <= (uint32_t)m_dump_unsynced_bytes)
        m_packet_data.clear();
    else
        m_packet_data.erase(m_packet_data.begin(), m_packet_data.begin() + m_dump_unsynced_bytes);

    m_dump_unsynced_bytes = 0;

    return resp;
}

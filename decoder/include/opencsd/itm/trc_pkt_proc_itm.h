/*
 * \file       trc_pkt_proc_itm.h
 * \brief      OpenCSD : ITM packet processing
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

#ifndef ARM_TRC_PKT_PROC_ITM_H_INCLUDED
#define ARM_TRC_PKT_PROC_ITM_H_INCLUDED

#include <vector>

#include "trc_pkt_types_itm.h"
#include "common/trc_pkt_proc_base.h"
#include "trc_pkt_elem_itm.h"
#include "trc_cmp_cfg_itm.h"

/* convert incoming byte stream into ITM packets for the ITM packet decoder */
class TrcPktProcItm : public TrcPktProcBase<ItmTrcPacket, ocsd_itm_pkt_type, ITMConfig>
{
public:
    TrcPktProcItm();
    TrcPktProcItm(int instIDNum);
    virtual ~TrcPktProcItm();

protected:
    /* implementation packet processing interface */
    virtual ocsd_datapath_resp_t processData(const ocsd_trc_index_t index,
        const uint32_t dataBlockSize,
        const uint8_t* pDataBlock,
        uint32_t* numBytesProcessed);
    virtual ocsd_datapath_resp_t onEOT();
    virtual ocsd_datapath_resp_t onReset();
    virtual ocsd_datapath_resp_t onFlush();
    virtual ocsd_err_t onProtocolConfig();
    virtual const bool isBadPacket() const;


    typedef enum _process_state {
        WAIT_SYNC,
        PROC_HDR,       // process header byte
        PROC_DATA,      // process payload bytes
        SEND_PKT
    } process_state;

    process_state m_proc_state;

private:
    void initObj();
    void initProcessorState();
    void initNextPacket();
    ocsd_datapath_resp_t waitForSync(const ocsd_trc_index_t blk_st_index);

    ocsd_datapath_resp_t outputPacket();   //!< send packet on output 
    void sendPacket();                      //!< mark packet for send.
    void setProcUnsynced();                 //!< set processor state to unsynced
    void throwBadSequenceError(const char* pszMessage = "");
    void throwReservedHdrError(const char* pszMessage = "");

    // packet processing functions
    void itmProcessHdr();
    void itmPktAsync();
    void itmPktLocalTS();
    void itmPktGlobalTS1();
    void itmPktGlobalTS2();
    void itmPktExtension();
    void itmPktData();

    bool readContBytes(uint32_t limit);
    uint32_t extractContVal32();
    uint64_t extractContVal64();
    bool readAsyncSeq(bool &bError); // look for an async sequence from current data position
    ocsd_datapath_resp_t flushUnsyncedBytes();

    typedef void (TrcPktProcItm::* PPKTFN)(void);
    PPKTFN m_pCurrPktFn;    // current active processing function.

    const bool dataToProcess() const;
    void savePacketByte(const uint8_t val);
    const bool readByte(uint8_t &byte);

    ItmTrcPacket m_curr_packet;             //!< current packet created from input data
    bool m_bStreamSync;                     //!< packet stream is synced

    const uint8_t* m_p_data_in;             //!< pointer to input data.
    uint32_t  m_data_in_size;               //!< amount of data in.
    uint32_t  m_data_in_used;               //!< amount of data processed.
    ocsd_trc_index_t m_packet_index;        //!< byte index for start of current packet

    uint8_t m_header_byte;                  //!< header byte for current packet    
    std::vector<uint8_t> m_packet_data;     //!< current packet data (bytes)

    bool m_sent_notsync_packet;             //!< flag in case we don't see async first up
    bool m_sync_start;                      //!< flag to say we have seen a sync start
    int m_dump_unsynced_bytes;              //!< unsynced bytes to dump

};


inline void TrcPktProcItm::setProcUnsynced()
{
    m_proc_state = WAIT_SYNC;
    m_bStreamSync = false;
}

inline void TrcPktProcItm::sendPacket()
{
    m_proc_state = SEND_PKT;
}

inline void TrcPktProcItm::savePacketByte(const uint8_t val)
{
    // save packet data if using monitor and synchronised.
    m_packet_data.push_back(val);
}

inline const bool TrcPktProcItm::dataToProcess() const
{
    // data to process if
    // 1) not processed all the input bytes
    // 2) bytes processed, but there is a full packet to send
    return (m_data_in_used < m_data_in_size) || (m_proc_state == SEND_PKT);
}

#endif // ARM_TRC_PKT_PROC_ITM_H_INCLUDED

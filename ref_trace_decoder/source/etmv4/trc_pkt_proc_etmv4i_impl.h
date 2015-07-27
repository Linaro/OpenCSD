/*
 * \file       trc_pkt_proc_etmv4i_impl.h
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

#ifndef ARM_TRC_PKT_PROC_ETMV4I_IMPL_H_INCLUDED
#define ARM_TRC_PKT_PROC_ETMV4I_IMPL_H_INCLUDED

#include "etmv4/trc_pkt_proc_etmv4.h"
#include "etmv4/trc_cmp_cfg_etmv4.h"
#include "etmv4/trc_pkt_elem_etmv4i.h"

class EtmV4IPktProcImpl
{
public:
    EtmV4IPktProcImpl();
    ~EtmV4IPktProcImpl();

    void Initialise(TrcPktProcEtmV4I *p_interface);

    rctdl_err_t Configure(const EtmV4Config *p_config);


    rctdl_datapath_resp_t processData(  const rctdl_trc_index_t index,
                                        const uint32_t dataBlockSize,
                                        const uint8_t *pDataBlock,
                                        uint32_t *numBytesProcessed);
    rctdl_datapath_resp_t onEOT();
    rctdl_datapath_resp_t onReset();
    rctdl_datapath_resp_t onFlush();

protected:
    typedef enum _process_state {
        PROC_HDR,
        PROC_DATA,
        SEND_PKT, 
        PROC_ERR,
    } process_state;
    
    process_state m_process_state;

    void InitPacketState();      // clear current packet state.
    void InitProcessorState();   // clear all previous process state

    /** configuration **/
    bool m_isInit;
    TrcPktProcEtmV4I *m_interface;       /**< The interface to the other decode components */
    
    EtmV4Config m_config;
    uint8_t m_chanIDCopy;


    /** packet data **/
    std::vector<uint8_t> m_currPacketData;  // raw data
    int m_currPktIdx;   // index into raw packet when expanding
    EtmV4ITrcPacket m_curr_packet;  // expanded packet
    rctdl_trc_index_t m_packet_index;   // index of the start of the current packet

    bool m_is_sync;             //!< seen first sync packet
    bool m_first_trace_info;    //!< seen first trace info packet after sync

private:
    rctdl_datapath_resp_t outputPacket();
    void outputUnsyncedRawPacket(int nbytes); // unsynced packets will not cause data path stall.

    void iNotSync();      // not synced yet
    void iPktNoPayload(); // process a single byte packet
    void iPktReserved();  // deal with reserved header value
    void iPktExtension();
    void iPktASync();
    void iPktTraceInfo();
    void iPktTimestamp();
    void iPktException();
    void iPktCycleCntF123();
    void iPktSpeclRes();
    void iPktCondInstr();
    void iPktCondResult();
    void iPktContext();
    void iPktAddrCtxt();
    void iPktShortAddr();
    void iPktLongAddr();
    void iPktQ();
    void iAtom();

    int extractContField(const std::vector<uint8_t> &buffer, const int st_idx, uint32_t &value, const int byte_limit = 5);
    int extractContField64(const std::vector<uint8_t> &buffer, const int st_idx, uint64_t &value, const int byte_limit = 9);
    int extractCondResult(const std::vector<uint8_t> &buffer, const int st_idx, uint32_t& key, uint8_t &result);
    void extractAndSetContextInfo(const std::vector<uint8_t> &buffer, const int st_idx);
    int extract64BitLongAddr(const std::vector<uint8_t> &buffer, const int st_idx, const uint8_t IS, uint64_t &value);
    int extract32BitLongAddr(const std::vector<uint8_t> &buffer, const int st_idx, const uint8_t IS, uint32_t &value);
    int extractShortAddr(const std::vector<uint8_t> &buffer, const int st_idx, const uint8_t IS, uint32_t &value, int &bits);

    // packet processing is table driven.    
    typedef void (EtmV4IPktProcImpl::*PPKTFN)(void);
    PPKTFN m_pIPktFn;

    struct _pkt_i_table_t {
        rctdl_etmv4_i_pkt_type pkt_type;
        PPKTFN pptkFn;
    } m_i_table[256];

    void BuildIPacketTable();
};

#endif // ARM_TRC_PKT_PROC_ETMV4I_IMPL_H_INCLUDED

/* End of File trc_pkt_proc_etmv4i_impl.h */

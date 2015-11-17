/*
 * \file       trc_pkt_proc_stm.h
 * \brief      Reference CoreSight Trace Decoder : STM packet processing
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

#ifndef ARM_TRC_PKT_PROC_STM_H_INCLUDED
#define ARM_TRC_PKT_PROC_STM_H_INCLUDED

#include <vector>

#include "trc_pkt_types_stm.h"
#include "trc_pkt_proc_base.h"
#include "trc_pkt_elem_stm.h"
#include "trc_cmp_cfg_stm.h"

/** @addtogroup rctdl_pkt_proc
@{*/

class TrcPktProcStm : public TrcPktProcBase<StmTrcPacket, rctdl_stm_pkt_type, STMConfig>
{
public:
    TrcPktProcStm();
    TrcPktProcStm(int instIDNum);
    virtual ~TrcPktProcStm();

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

private:

    // packet processing routines
    // 1 nibble opcodes
    void stmPktReserved();
    void stmPktNull();
    void stmPktM8();
    void stmPktMERR();
    void stmPktC8();
    void stmPktD8();
    void stmPktD16();
    void stmPktD32();
    void stmPktD64();
    void stmPktD8MTS();
    void stmPktD16MTS();
    void stmPktD32MTS();
    void stmPktD64MTS();
    void stmPktFlagTS();
    void stmPktFExt();

    // 2 nibble opcodes 0xFn
    void stmPktReservedFn();
    void stmPktF0Ext();
    void stmPktGERR();
    void stmPktC16();
    void stmPktD8TS();
    void stmPktD16TS();
    void stmPktD32TS();
    void stmPktD64TS();
    void stmPktD8M();
    void stmPktD16M();
    void stmPktD32M();
    void stmPktD64M();
    void stmPktFlag();
    void stmPktASync();

    // 3 nibble opcodes 0xF0n
    void stmPktReservedF0n();
    void stmPktVersion();
    void stmPktTrigger();
    void stmPktTriggerTS();
    void stmPktFreq();
    
    void stmExtractTS(); // extract a TS in packets that require it.
    void stmExtractVal8(uint8_t nibbles_to_val);
    void stmExtractVal16(uint8_t nibbles_to_val);
    void stmExtractVal32(uint8_t nibbles_to_val);
    void stmExtractVal64(uint8_t nibbles_to_val);

    uint64_t bin_to_gray(uint64_t bin_value);
    uint64_t gray_to_bin(uint64_t gray_value);

    // data processing op function tables
    void buildOpTables();

    typedef void (TrcPktProcStm::*PPKTFN)(void);
    PPKTFN m_pCurrPktFn;    // current active processing function.

    PPKTFN m_1N_ops[0x10];
    PPKTFN m_2N_ops[0x10];
    PPKTFN m_3N_ops[0x10];

    // read a nibble from the input data - may read a byte and set spare or return spare.
    // handles setting up packet data block and end of input 
    bool readNibble();


    // packet data 
    StmTrcPacket m_curr_packet;     //!< current packet.
    bool m_bNeedsTS;                //!< packet requires a TS
    bool m_bIsMarker;

    uint8_t  m_num_nibbles;                 //!< number of nibbles in the current packet
    std::vector<uint8_t> m_packet_data;     //!< current packet data (bytes).
    uint8_t  m_nibble;                      //!< current nibble being processed.
    uint8_t  m_nibble_spare;                //!< unused nibble from a processed byte.
    bool     m_spare_valid;                 //!< spare nibble is valid;
    uint8_t  m_num_data_nibbles;            //!< number of nibbles needed to acheive payload.

    uint8_t  *m_p_data_in;                  //!< pointer to input data.
    uint32_t  m_data_in_size;               //!< amount of data in.
    uint32_t  m_data_in_used;               //!< amount of data processed.

    uint8_t   m_val8;                       //!< 8 bit payload.
    uint16_t  m_val16;                      //!< 16 bit payload
    uint32_t  m_val32;                      //!< 32 bit payload
    uint64_t  m_val64;                      //!< 64 bit payload
};

/** @}*/

#endif // ARM_TRC_PKT_PROC_STM_H_INCLUDED

/* End of File trc_pkt_proc_stm.h */

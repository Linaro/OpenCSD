/*
 * \file       trc_pkt_proc_stm.cpp
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

#include "stm/trc_pkt_proc_stm.h"

TrcPktProcStm::TrcPktProcStm()
{
}

TrcPktProcStm::TrcPktProcStm(int instIDNum)
{
}

TrcPktProcStm::~TrcPktProcStm()
{
}


/* implementation packet processing interface */
rctdl_datapath_resp_t TrcPktProcStm::processData(  const rctdl_trc_index_t index,
                                            const uint32_t dataBlockSize,
                                            const uint8_t *pDataBlock,
                                            uint32_t *numBytesProcessed)
{
}

rctdl_datapath_resp_t onEOT()
{
}

rctdl_datapath_resp_t TrcPktProcStm::onReset()
{
}

rctdl_datapath_resp_t TrcPktProcStm::onFlush()
{
}

rctdl_err_t TrcPktProcStm::onProtocolConfig()
{
}

const bool TrcPktProcStm::isBadPacket() const
{
}

// packet processing routines
// ************************
// 1 nibble opcodes
void TrcPktProcStm::stmPktReserved()
{
    uint16_t bad_opcode = (uint16_t)m_nibble;
    m_curr_packet.setPacketType(STM_PKT_RESERVED,false);
    m_curr_packet.setD16Payload(bad_opcode);

    // send packet immediately
}

void TrcPktProcStm::stmPktNull()
{
    m_curr_packet.setPacketType(STM_PKT_NULL,false);
    // send packet immediately
}

void TrcPktProcStm::stmPktM8()
{
    bool bCont = true;
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
        m_curr_packet.setPacketType(STM_PKT_M8,false);

    stmExtractVal8(3);
    if(m_num_nibbles == 3)
    {
        m_curr_packet.setMaster(m_val8);
        // send packet
    }
}

void TrcPktProcStm::stmPktMERR()
{
    bool bCont = true;
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
        m_curr_packet.setPacketType(STM_PKT_MERR,false);

    stmExtractVal8(3);
    if(m_num_nibbles == 3)
    {
        m_curr_packet.setD8Payload(m_val8);
        // send packet
    }

}

void TrcPktProcStm::stmPktC8()
{
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
        m_curr_packet.setPacketType(STM_PKT_C8,false);
    stmExtractVal8(3);
    if(m_num_nibbles == 3)
    {
        m_curr_packet.setChannel((uint16_t)m_val8,true);
        // send packet
    }
}

void TrcPktProcStm::stmPktD8()
{
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
    {
        m_curr_packet.setPacketType(STM_PKT_D8,false);
        m_num_data_nibbles = 3;
    }

    stmExtractVal8(m_num_data_nibbles);
    if(m_num_nibbles == m_num_data_nibbles)
    {
        m_curr_packet.setD8Payload(m_val8);
        if(m_bNeedsTS)
        {
            m_pCurrPktFn = &TrcPktProcStm::stmExtractTS;
            (this->*m_pCurrPktFn)();
        }
        else
        {
            // send packet
        }
    }
}

void TrcPktProcStm::stmPktD16()
{
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
    {
        m_curr_packet.setPacketType(STM_PKT_D16,false);
        m_num_data_nibbles = 5;
    }

    stmExtractVal16(m_num_data_nibbles);
    if(m_num_nibbles == m_num_data_nibbles)
    {
        m_curr_packet.setD16Payload(m_val16);
        if(m_bNeedsTS)
        {
            m_pCurrPktFn = &TrcPktProcStm::stmExtractTS;
            (this->*m_pCurrPktFn)();
        }
        else
        {
            // send packet
        }
    }
}

void TrcPktProcStm::stmPktD32()
{
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
    {
        m_curr_packet.setPacketType(STM_PKT_D32,false);
        m_num_data_nibbles = 9;
    }

    stmExtractVal32(m_num_data_nibbles);
    if(m_num_nibbles == m_num_data_nibbles)
    {
        m_curr_packet.setD32Payload(m_val32);
        if(m_bNeedsTS)
        {
            m_pCurrPktFn = &TrcPktProcStm::stmExtractTS;
            (this->*m_pCurrPktFn)();
        }
        else
        {
            // send packet
        }
    }
}

void TrcPktProcStm::stmPktD64()
{
    if(m_num_nibbles == 1)    // 1st nibble - header - set type
    {
        m_curr_packet.setPacketType(STM_PKT_D64,false);
        m_num_data_nibbles = 17;
    }

    stmExtractVal64(m_num_data_nibbles);
    if(m_num_nibbles == m_num_data_nibbles)
    {
        m_curr_packet.setD64Payload(m_val64);
        if(m_bNeedsTS)
        {
            m_pCurrPktFn = &TrcPktProcStm::stmExtractTS;
            (this->*m_pCurrPktFn)();
        }
        else
        {
            // send packet
        }
    }
}

void TrcPktProcStm::stmPktD8MTS()
{
    m_bNeedsTS = true;
    m_bIsMarker = true;  
    m_pCurrPktFn = &TrcPktProcStm::stmPktD8;
    (this->*m_pCurrPktFn)();
}

void TrcPktProcStm::stmPktD16MTS()
{
    m_bNeedsTS = true;
    m_bIsMarker = true;    
    m_pCurrPktFn = &TrcPktProcStm::stmPktD16;
    (this->*m_pCurrPktFn)();
}

void TrcPktProcStm::stmPktD32MTS()
{
    m_bNeedsTS = true;
    m_bIsMarker = true;    
    m_pCurrPktFn = &TrcPktProcStm::stmPktD32;
    (this->*m_pCurrPktFn)();
}

void TrcPktProcStm::stmPktD64MTS()
{
    m_bNeedsTS = true;
    m_bIsMarker = true;    
    m_pCurrPktFn = &TrcPktProcStm::stmPktD64;
    (this->*m_pCurrPktFn)();
}

void TrcPktProcStm::stmPktFlagTS()
{
    m_bNeedsTS = true;
    m_curr_packet.setPacketType(STM_PKT_FLAG,false);
    m_pCurrPktFn = &TrcPktProcStm::stmExtractTS;
    (this->*m_pCurrPktFn)();
}

void TrcPktProcStm::stmPktFExt()
{
    // no type, look at the next nibble
    if(readNibble())
    {
        // switch in 2N function
        m_pCurrPktFn = m_2N_ops[m_nibble];
        (this->*m_pCurrPktFn)();        
    }
}

// ************************
// 2 nibble opcodes 0xFn
void TrcPktProcStm::stmPktReservedFn()
{
    uint16_t bad_opcode = 0x00F;
    m_curr_packet.setPacketType(STM_PKT_RESERVED,false);
    bad_opcode |= ((uint16_t)m_nibble) << 4;
    m_curr_packet.setD16Payload(bad_opcode);

    // send packet immediately
}

void TrcPktProcStm::stmPktF0Ext()
{
    // no type yet, look at the next nibble
    if(readNibble())
    {
        // switch in 2N function
        m_pCurrPktFn = m_3N_ops[m_nibble];
        (this->*m_pCurrPktFn)();        
    }
}

void TrcPktProcStm::stmPktGERR()
{
    if(m_num_nibbles == 2)    // 2nd nibble - header - set type
        m_curr_packet.setPacketType(STM_PKT_GERR,false); 
    stmExtractVal8(4);
    if(m_num_nibbles == 4)
    {
        m_curr_packet.setD8Payload(m_val8);
        // send packet
    }
}

void TrcPktProcStm::stmPktC16()
{
    if(m_num_nibbles == 2)    // 2nd nibble - header - set type
        m_curr_packet.setPacketType(STM_PKT_C16,false);    
    stmExtractVal16(6);
    if(m_num_nibbles == 6)
    {
        m_curr_packet.setChannel(m_val16,false);
        // send packet
    }
}

void TrcPktProcStm::stmPktD8TS()
{
    m_bNeedsTS = true;
    m_curr_packet.setPacketType(STM_PKT_D8,false); // 2nd nibble, set type here
    m_num_data_nibbles = 4;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD8;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD16TS()
{
    m_bNeedsTS = true;
    m_curr_packet.setPacketType(STM_PKT_D16,false); // 2nd nibble, set type here
    m_num_data_nibbles = 6;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD16;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD32TS()
{
    m_bNeedsTS = true;
    m_curr_packet.setPacketType(STM_PKT_D32,false); // 2nd nibble, set type here
    m_num_data_nibbles = 10;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD32;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD64TS()
{
    m_bNeedsTS = true;
    m_curr_packet.setPacketType(STM_PKT_D64,false); // 2nd nibble, set type here
    m_num_data_nibbles = 18;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD64;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD8M()
{
    m_curr_packet.setPacketType(STM_PKT_D8,true); // 2nd nibble, set type here
    m_num_data_nibbles = 4;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD8;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD16M()
{
    m_curr_packet.setPacketType(STM_PKT_D16,true);
    m_num_data_nibbles = 6;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD16;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD32M()
{
    m_curr_packet.setPacketType(STM_PKT_D32,true);
    m_num_data_nibbles = 10;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD32;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktD64M()
{
    m_curr_packet.setPacketType(STM_PKT_D64,true);
    m_num_data_nibbles = 18;
    m_pCurrPktFn = &TrcPktProcStm::stmPktD64;
    (this->*m_pCurrPktFn)();  
}

void TrcPktProcStm::stmPktFlag()
{
    m_curr_packet.setPacketType(STM_PKT_FLAG,false);
    // send packet immediately
}

// ************************
// 3 nibble opcodes 0xF0n
void TrcPktProcStm::stmPktReservedF0n()
{
    uint16_t bad_opcode = 0x00F;
    m_curr_packet.setPacketType(STM_PKT_RESERVED,false);
    bad_opcode |= ((uint16_t)m_nibble) << 8;
    m_curr_packet.setD16Payload(bad_opcode);

    // send packet immediately
}

void TrcPktProcStm::stmPktVersion()
{
    if(m_num_nibbles == 3)
        m_curr_packet.setPacketType(STM_PKT_VERSION,false);

    if(readNibble())
    {
        m_curr_packet.setD8Payload(m_nibble);   // record the version number
        switch(m_nibble)
        {
        case 3:
            m_curr_packet.onVersionPkt(STM_TS_NATBINARY); break;            
        case 4: 
            m_curr_packet.onVersionPkt(STM_TS_GREY); break;
        default:
            // not a version we support.
            m_curr_packet.updateErrType(STM_PKT_BAD_SEQUENCE); break;        
        }
        // send the packet.
    }

}

void TrcPktProcStm::stmPktTrigger()
{
    if(m_num_nibbles == 3)
        m_curr_packet.setPacketType(STM_PKT_TRIG,false);
    stmExtractVal8(5);
    if(m_num_nibbles == 5)
    {
        m_curr_packet.setD8Payload(m_val8);
        if(m_bNeedsTS)
        {
            m_pCurrPktFn = &TrcPktProcStm::stmExtractTS;
            (this->*m_pCurrPktFn)();
        }
        else
        {
            // send packet
        }
    }
}

void TrcPktProcStm::stmPktTriggerTS()
{
    m_bNeedsTS = true;
    m_pCurrPktFn = &TrcPktProcStm::stmPktTrigger;
    (this->*m_pCurrPktFn)(); 
}

void TrcPktProcStm::stmPktFreq()
{
    if(m_num_nibbles == 3)
        m_curr_packet.setPacketType(STM_PKT_FREQ,false);
    stmExtractVal32(11);
    if(m_num_nibbles == 11)
    {
        m_curr_packet.setD32Payload(m_val32);
        // send packet
    }
}

void TrcPktProcStm::stmPktASync()
{
    // 2 nibbles - 0xFF - must be an async or error.
}

// ************************
// general data processing

// return false if no more data
// in an STM byte, 3:0 is 1st nibble in protocol order, 7:4 is 2nd nibble.
bool TrcPktProcStm::readNibble()
{
    bool dataFound = true;
    if(m_spare_valid)
    {
        m_nibble = m_nibble_spare;
        m_spare_valid = false;
        m_num_nibbles++;
        /* TBD: do we really want to do this ??? 
        if(m_packet_data.size() == 0) // spare a header - reinsert
        {
            value <<= 4;    // move to 2nd position, leave "null" in 1st position if printing out / re-using
            m_packet_data.push_back(value);
        }
        */
    }
    else if( m_data_in_size < m_data_in_used)
    {
        m_nibble = m_p_data_in[m_data_in_used++];
        m_packet_data.push_back(m_nibble);
        m_nibble_spare = (m_nibble >> 4) & 0xF;
        m_spare_valid = true;
        m_nibble &= 0xF;
        m_num_nibbles++;
    }
    else
        dataFound = false;  // no data available
    return dataFound;
}
    
void TrcPktProcStm::stmExtractTS()
{
}

// pass in number of nibbles needed to extract the value
void TrcPktProcStm::stmExtractVal8(uint8_t nibbles_to_val)
{
    bool bCont = true;
    while(bCont && (m_num_nibbles < nibbles_to_val))
    {
        bCont = readNibble();
        if(bCont)   // got a nibble
        {
            m_val8 <<= 4;
            m_val8 |= m_nibble;
        }
    }
}

void TrcPktProcStm::stmExtractVal16(uint8_t nibbles_to_val)
{
    bool bCont = true;
    while(bCont && (m_num_nibbles < nibbles_to_val))
    {
        bCont = readNibble();
        if(bCont)   // got a nibble
        {
            m_val16 <<= 4;
            m_val16 |= m_nibble;
        }
    }
}

void TrcPktProcStm::stmExtractVal32(uint8_t nibbles_to_val)
{
    bool bCont = true;
    while(bCont && (m_num_nibbles < nibbles_to_val))
    {
        bCont = readNibble();
        if(bCont)   // got a nibble
        {
            m_val32 <<= 4;
            m_val32 |= m_nibble;
        }
    }
}

void TrcPktProcStm::stmExtractVal64(uint8_t nibbles_to_val)
{
    bool bCont = true;
    while(bCont && (m_num_nibbles < nibbles_to_val))
    {
        bCont = readNibble();
        if(bCont)   // got a nibble
        {
            m_val64 <<= 4;
            m_val64 |= m_nibble;
        }
    }
}

uint64_t TrcPktProcStm::bin_to_gray(uint64_t bin_value)
{
	uint64_t gray_value = 0;
	gray_value = (1ull << 63) & bin_value;
	int i = 62;
	for (; i >= 0; i--) {
		uint64_t gray_arg_1 = ((1ull << (i+1)) & bin_value) >> (i+1);
		uint64_t gray_arg_2 = ((1ull << i) & bin_value) >> i;
		gray_value |= ((gray_arg_1 ^ gray_arg_2) << i);
	}
	return gray_value;
}

uint64_t TrcPktProcStm::gray_to_bin(uint64_t gray_value)
{
	uint64_t bin_value = 0;
	int bin_bit = 0;
	for (; bin_bit < 64; bin_bit++) {
		uint8_t bit_tmp = ((1ull << bin_bit) & gray_value) >> bin_bit;
		uint8_t gray_bit = bin_bit + 1;
		for (; gray_bit < 64; gray_bit++)
			bit_tmp ^= (((1ull << gray_bit) & gray_value) >> gray_bit);

		bin_value |= (bit_tmp << bin_bit);
	}

	return bin_value;
}


void TrcPktProcStm::buildOpTables()
{
    // init all reserved
    for(int i = 0; i < 0x10; i++)
    {
        m_1N_ops[i] = &TrcPktProcStm::stmPktReserved;
        m_2N_ops[i] = &TrcPktProcStm::stmPktReservedFn;
        m_3N_ops[i] = &TrcPktProcStm::stmPktReservedF0n;
    }

    // set the 1N operations
    m_1N_ops[0x0] = &TrcPktProcStm::stmPktNull;
    m_1N_ops[0x1] = &TrcPktProcStm::stmPktM8;
    m_1N_ops[0x2] = &TrcPktProcStm::stmPktMERR;
    m_1N_ops[0x3] = &TrcPktProcStm::stmPktC8;
    m_1N_ops[0x4] = &TrcPktProcStm::stmPktD8;
    m_1N_ops[0x5] = &TrcPktProcStm::stmPktD16;
    m_1N_ops[0x6] = &TrcPktProcStm::stmPktD32;
    m_1N_ops[0x7] = &TrcPktProcStm::stmPktD64;
    m_1N_ops[0x8] = &TrcPktProcStm::stmPktD8MTS;
    m_1N_ops[0x9] = &TrcPktProcStm::stmPktD16MTS;
    m_1N_ops[0xA] = &TrcPktProcStm::stmPktD32MTS;
    m_1N_ops[0xB] = &TrcPktProcStm::stmPktD64MTS;
    // 0xC & 0xD not used by CS STM.
    m_1N_ops[0xE] = &TrcPktProcStm::stmPktFlagTS;
    m_1N_ops[0xF] = &TrcPktProcStm::stmPktFExt;

    // set the 2N operations 0xFn
    m_2N_ops[0x0] = &TrcPktProcStm::stmPktF0Ext;
    // 0x1 unused in CS STM
    m_2N_ops[0x2] = &TrcPktProcStm::stmPktGERR;
    m_2N_ops[0x3] = &TrcPktProcStm::stmPktC16;
    m_2N_ops[0x4] = &TrcPktProcStm::stmPktD8TS;
    m_2N_ops[0x5] = &TrcPktProcStm::stmPktD16TS;
    m_2N_ops[0x6] = &TrcPktProcStm::stmPktD32TS;
    m_2N_ops[0x7] = &TrcPktProcStm::stmPktD64TS;
    m_2N_ops[0x8] = &TrcPktProcStm::stmPktD8M;
    m_2N_ops[0x9] = &TrcPktProcStm::stmPktD16M;
    m_2N_ops[0xA] = &TrcPktProcStm::stmPktD32M;
    m_2N_ops[0xB] = &TrcPktProcStm::stmPktD64M;
    // 0xC & 0xD not used by CS STM.
    m_2N_ops[0xE] = &TrcPktProcStm::stmPktFlag;
    m_2N_ops[0xF] = &TrcPktProcStm::stmPktASync;

    // set the 3N operations 0xF0n
    m_3N_ops[0x0] = &TrcPktProcStm::stmPktVersion;
    // 0x1 .. 0x5 not used by CS STM
    m_3N_ops[0x6] = &TrcPktProcStm::stmPktTrigger;
    m_3N_ops[0x7] = &TrcPktProcStm::stmPktTriggerTS;
    m_3N_ops[0x8] = &TrcPktProcStm::stmPktFreq;
    // 0x9 .. 0xF not used by CS STM

}

/* End of File trc_pkt_proc_stm.cpp */

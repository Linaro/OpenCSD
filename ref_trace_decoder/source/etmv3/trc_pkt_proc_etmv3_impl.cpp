/*
 * \file       trc_pkt_proc_etmv3_impl.cpp
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

#include "trc_pkt_proc_etmv3_impl.h"

EtmV3PktProcImpl::EtmV3PktProcImpl() :
    m_isInit(false),
    m_interface(0)
{
}

EtmV3PktProcImpl::~EtmV3PktProcImpl()
{
}

rctdl_err_t EtmV3PktProcImpl::Configure(const EtmV3Config *p_config)
{
    rctdl_err_t err = RCTDL_OK;
    if(p_config != 0)
    {
        m_config = *p_config;
        m_chanIDCopy = m_config.getTraceID();
    }
    else
    {
        err = RCTDL_ERR_INVALID_PARAM_VAL;
        if(m_isInit)
            m_interface->LogError(rctdlError(RCTDL_ERR_SEV_ERROR,err));
    }
    return err;
}

rctdl_datapath_resp_t EtmV3PktProcImpl::processData(const rctdl_trc_index_t index,
                                                    const uint32_t dataBlockSize,
                                                    const uint8_t *pDataBlock,
                                                    uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    uint32_t bytesProcessed = 0;

    while((bytesProcessed < dataBlockSize) && RCTDL_DATA_RESP_IS_CONT(resp))
    {
        try 
        {
            switch(m_process_state)
            {
            case WAIT_SYNC:
                bytesProcessed += waitForSync(index, dataBlockSize,pDataBlock);
                break;

            case PROC_HDR:
                m_packet_index = index +  bytesProcessed;
                processHeaderByte(pDataBlock[bytesProcessed++]);
                break;

            case PROC_DATA:
                processPayloadByte(pDataBlock [bytesProcessed++]);
                break;

            case SEND_PKT:
                resp =  outputPacket();
                break;
            }
        }
        catch(rctdlError &err)
        {
            m_interface->LogError(err);
            m_process_state = PROC_HDR;

            /// TBD - determine what to do with the error - depends on error and opmode.
        }
        catch(...)
        {
            /// vv bad at this point.
            resp = RCTDL_RESP_FATAL_SYS_ERR;
            rctdlError &fatal = rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_FAIL,m_packet_index,m_chanIDCopy);
            fatal.setMessage("Unknown System Error decoding trace.");
            m_interface->LogError(fatal);
        }
    }

    *numBytesProcessed = bytesProcessed;
    return resp;
}

rctdl_datapath_resp_t EtmV3PktProcImpl::onEOT()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t EtmV3PktProcImpl::onReset()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

rctdl_datapath_resp_t EtmV3PktProcImpl::onFlush()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    return resp;
}

 void EtmV3PktProcImpl::Initialise(TrcPktProcEtmV3 *p_interface)
 {
     if(p_interface)
     {
         m_interface = p_interface;
         m_isInit = true;
         
     }
     static const uint8_t a_sync[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 };
     m_syncMatch.setPattern(a_sync, sizeof(a_sync));
 }


void EtmV3PktProcImpl::InitProcessorState()
{
    m_bStreamSync = false;
    InitPacketState();
    m_process_state = WAIT_SYNC;
}

void EtmV3PktProcImpl::InitPacketState()
{
    m_bytesExpectedThisPkt = 0; 
	m_BranchPktNeedsException = false;
	m_bIsync_got_cycle_cnt = false;
	m_bIsync_get_LSiP_addr = false;
	m_IsyncInfoIdx = false;
	m_bExpectingDataAddress = false;
	m_bFoundDataAddress = false;
    m_currPacketData.clear();
    m_curr_packet.Clear();
    
}

rctdl_datapath_resp_t EtmV3PktProcImpl::outputPacket()
{
    rctdl_datapath_resp_t dp_resp = RCTDL_RESP_FATAL_NOT_INIT;
    if(m_isInit)
    {
        if(!m_bSendPartPkt) 
        {
            dp_resp = m_interface->outputOnAllInterfaces(m_packet_index,&m_curr_packet,&m_curr_packet.type,m_currPacketData);
            m_process_state = PROC_HDR; // need a header next time.
        }
        else
        {
            // sending part packet, still some data in the main packet
            dp_resp = m_interface->outputOnAllInterfaces(m_packet_index,&m_curr_packet,&m_curr_packet.type,m_partPktData);
            m_process_state = m_post_part_pkt_state;
            m_packet_index += m_partPktData.size();
            m_bSendPartPkt = false;
        }
    }
    return dp_resp;
}

void EtmV3PktProcImpl::moveBytesPartPkt(int n)
{
    m_partPktData.clear();
    for(int i=0; i < n; i++)
    {
        m_partPktData.push_back(m_currPacketData[i]);
    }
    m_currPacketData.erase(m_currPacketData.begin(), m_currPacketData.begin()+n);
    m_bSendPartPkt = true;
}

int EtmV3PktProcImpl::waitForSync(const rctdl_trc_index_t index, const uint32_t dataBlockSize, const uint8_t *pDataBlock)
{
    // if the block is larger than an ASYNC, then use the pattern matcher, otherwise byte by byte.
    
    
    
    if(m_bPartSync)
        m_syncMatch.checkContinuation(pDataBlock,dataBlockSize,&m_currPacketData[0],m_currPacketData.size());
    else
        m_syncMatch.checkBuffer(pDataBlock,dataBlockSize);
        
    if(m_syncMatch.found_full())
    {
        // send unsynced  bytes up to m_syncMatch.pos()

        
    }
}


rctdl_err_t  EtmV3PktProcImpl::processHeaderByte(uint8_t by)
{

    m_currPacketData.clear();
    m_currPacketData.push_back(by);
    m_curr_packet.Clear();  // new packet - clear old update data.
    m_currPktIdx = 0;       // index into processed bytes in current packet
    m_process_state = PROC_DATA;    // assume next is data packet

	// check for branch address 0bCxxxxxxx1
	if((by & 0x01) == 0x01 ) {
		m_curr_packet.type = PKT_BRANCH_ADDRESS;
		m_BranchPktNeedsException = false;
		if((by & 0x80) != 0x80) {
			// no continuation - 1 byte branch same in alt and std...
			if(by == 0x01) // could be EOTrace marker from bypassed formatter
			{
				m_curr_packet.type = PKT_BRANCH_OR_BYPASS_EOT;
			}
			else
            {
                OnBranchAddress();
				SendPacket();  // mark ready to send.
            }
		}
	}
	// check for p-header - 0b1xxxxxx0
	else if((by & 0x81) == 0x80) {
		m_curr_packet.type = PKT_P_HDR;
		SendPacket();
	}
	// check 0b0000xx00 group
	else if((by & 0xF3) == 0x00) {
			
		// 	A-Sync
		if(by == 0x00) {
			m_curr_packet.type = PKT_A_SYNC;
		}
		// cycle count
		else if(by == 0x04) {
			m_curr_packet.type = PKT_CYCLE_COUNT;
		}
		// I-Sync
		else if(by == 0x08) {
			m_curr_packet.type = PKT_I_SYNC;
			m_bIsync_got_cycle_cnt = false;
			m_bIsync_get_LSiP_addr = false;							
		}
		// trigger
		else if(by == 0x0C) {
			m_curr_packet.type = PKT_TRIGGER;
			SendPacket();
		}
	}
	// check remaining 0bxxxxxx00 codes
	else if((by & 0x03 )== 0x00) {
		// OoO data 0b0xx0xx00
		if((by & 0x93 )== 0x00) {
            if(!m_config.isDataTrace()) {
                m_curr_packet.type = PKT_BAD_TRACEMODE;
                LogError(TCERR_BAD_PCKT_MODE,"ERROR : Invalid data trace header - not tracing data.");
                SendPacket();
				return;
			}
			m_curr_packet.type = PKT_OOO_DATA;
			uint8_t size = ((by & 0x0C) >> 2);
			// header contains a count of the data to follow
			// size 3 == 4 bytes, other sizes == size bytes
			if(size == 0)
				SendPacket();
			else
				m_bytesExpectedThisPkt = (short)(1 + ((size == 3) ? 4 : size));
		}
		// I-Sync + cycle count
		else if(by == 0x70) {
			m_curr_packet.type = PKT_I_SYNC_CYCLE;
			m_bIsync_got_cycle_cnt = false;
			m_bIsync_get_LSiP_addr = false;			
		}
		// store failed
		else if(by == 0x50) {
			m_curr_packet.type = PKT_STORE_FAIL;
			SendPacket();
		}
		// OoO placeholder 0b01x1xx00
		else if((by & 0xD3 )== 0x50) {
			m_curr_packet.type = PKT_OOO_ADDR_PLC;
			m_bExpectingDataAddress = ((by & DATAPKT_ADDREXPECTED_FLAG) == DATAPKT_ADDREXPECTED_FLAG) && m_config.isDataAddrTrace();
			m_bFoundDataAddress = false;
			if(!m_bExpectingDataAddress) {
				SendPacket();
			}
		}
		// aux data 0b0001xx00
		else if((by & 0xF3 )== 0x10) {
			m_curr_packet.type = PKT_AUX_DATA;
			SendPacket();
		}
        // vmid 0b00111100 
        else if(by == 0x3c) {
            m_curr_packet.type = PKT_VMID;
        }
		else
		{
            LogError(TCERR_BAD_PACKET,"Packet header reserved encoding\n");
			m_curr_packet.type = PKT_RESERVED;
			SendPacket();
		}
	}
	// normal data 0b00x0xx10
	else if((by & 0xD3 )== 0x02) {
		uint8_t size = ((by & 0x0C) >> 2);
		if(!m_config.isDataTrace()) {
            LogError(TCERR_BAD_PCKT_MODE,"ERROR : Invalid data trace header - not tracing data.");
            m_curr_packet.type = PKT_BAD_TRACEMODE;
            SendPacket();
			return;
		}
		m_curr_packet.type = PKT_NORM_DATA;
		m_bExpectingDataAddress = ((by & DATAPKT_ADDREXPECTED_FLAG) == DATAPKT_ADDREXPECTED_FLAG) && m_config.isDataAddrTrace();
		m_bFoundDataAddress = false;

        // set this with the data bytes expected this packet, plus the header byte.
		m_bytesExpectedThisPkt = (short)( 1 + ((size == 3) ? 4 : size));
		if(!m_bExpectingDataAddress && (m_bytesExpectedThisPkt == 1)) {
			// single byte data packet, value = 0;
			SendPacket();
		}

	}
	// data suppressed 0b01100010
	else if(by == 0x62) {
		if(!m_config.isDataTrace())
        {
            LogError(TCERR_BAD_PCKT_MODE,"ERROR : Invalid data trace header - not tracing data.");
            m_curr_packet.type = PKT_BAD_TRACEMODE;
            SendPacket();
        }
		else
        {
			m_curr_packet.type = PKT_DATA_SUPPRESSED;
            SendPacket();
        }
	}
	// value not traced 0b011x1010
	else if((by & 0xEF )== 0x6A) {
		if(!m_config.isDataTrace()) {
            LogError(TCERR_BAD_PCKT_MODE,"ERROR : Invalid data trace header - not tracing data.");
            m_curr_packet.type = PKT_BAD_TRACEMODE;
            SendPacket();
            return;
		}
		m_curr_packet.type = PKT_VAL_NOT_TRACED;
		m_bExpectingDataAddress = ((by & DATAPKT_ADDREXPECTED_FLAG) == DATAPKT_ADDREXPECTED_FLAG) && m_config.isDataAddrTrace();
		m_bFoundDataAddress = false;
		if(!m_bExpectingDataAddress) {
			SendPacket();
        }
	}
	// ignore 0b01100110
	else if(by == 0x66) {
		m_curr_packet.type = PKT_IGNORE;
        SendPacket();					
	}
	// context ID 0b01101110
	else if(by == 0x6E) {
		m_curr_packet.type = PKT_CONTEXT_ID;
        m_bytesExpectedThisPkt = (short)(1 + m_config.CtxtIDBytes());
	}
	// exception entry 0b01110110
	else if(by == 0x76) {
		m_curr_packet.type = PKT_EXCEPTION_ENTRY;
        SendPacket();
	}
	// exception exit 0b01111110
	else if(by == 0x7E) {
		m_curr_packet.type = PKT_EXCEPTION_EXIT;
        SendPacket();
	}
	// timestamp packet 0b01000x10
	else if((by & 0xFB )== 0x42)
	{
		m_curr_packet.type = PKT_TIMESTAMP;
	}
	else
	{
		m_curr_packet.type = PKT_RESERVED;
        LogError(TCERR_BAD_PACKET,"Packet header reserved encoding\n");
        SendPacket();
	}



}

rctdl_err_t  EtmV3PktProcImpl::processPayloadByte(uint8_t by)
{
    bool bTopBitSet = false;
    bool packetDone = false;

	// pop byte into buffer
    m_currPacketData.push_back(by);
				
    switch(m_curr_packet.type) {
	default:
        // TBD throw erroor here SendBadPacket("Error : Interpreter failed - unexpected or unsupported packet");
		break;
     	
	case PKT_BRANCH_ADDRESS:
		bTopBitSet = (bool)((by & 0x80) == 0x80);
        if(m_config.isAltBranch()) 
        {
			if(!bTopBitSet)     // no continuation
            {
				if(!m_BranchPktNeedsException)
				{
					if((by & 0xC0) == 0x40) 
						m_BranchPktNeedsException = true;
					else
                        packetDone = true;
				}
				else
                    packetDone = true;
			}				
		}
		else 
        {
			// standard encoding  < 5 bytes cannot be exception branch
			// 5 byte packet
            if(m_currPacketData.size() == 5) {
				if((by & 0xC0) == 0x40) 
					// expecting follow up byte(s)
					m_BranchPktNeedsException = true;
				else
                    packetDone = true;						
			}
			// waiting for exception packet
			else if(m_BranchPktNeedsException){
				if(!bTopBitSet)
					packetDone = true;
			}
			else {
				// not exception - end of packets
				if(!bTopBitSet)
					packetDone = true;				
			}
		}

        if(packetDone)
        {
            OnBranchAddress();
			SendPacket();
        }
		break;

	case PKT_BRANCH_OR_BYPASS_EOT:
		if((by != 0x00) || ( m_currPacketSize == PKT_BUFF_SIZE)) {
			if(by == 0x80 && ( m_currPacketSize == 7)) {
				// branch 0 followed by A-sync!
				m_currPacketSize = 1;
                m_curr_packet.type = PKT_BRANCH_ADDRESS;
				SendPacket();
                memcpy(m_currPacketData, &m_currPacketData[1],6);
				m_currPacketSize = 6;
                m_curr_packet.type = PKT_A_SYNC;
				SendPacket();
			}
			else if( m_currPacketSize == 2) {
				// branch followed by another byte
                m_currPacketSize = 1;
                m_curr_packet.type = PKT_BRANCH_ADDRESS;
                SendPacket();
                ProcessHeaderByte(by);
			}
			else if(by == 0x00) {
				// end of buffer...output something - incomplete / unknown.
				SendPacket();
			}
			else if(by == 0x01) {
				// 0x01 - 0x00 x N - 0x1
				// end of buffer...output something
                m_currPacketSize--;
                SendPacket();
				ProcessHeaderByte(by);					
			}
			else {
				// branch followed by unknown sequence
				int oldidx =  m_currPacketSize;
                m_currPacketSize = 1;
                m_curr_packet.type = PKT_BRANCH_ADDRESS;
				SendPacket();
                oldidx--;
                memcpy(m_currPacketData, &m_currPacketData[1],oldidx);
                m_currPacketSize = oldidx;
                SendBadPacket("ERROR : unknown sequence");
			}
		}
		// just ignore zeros
		break;
			
		

	case PKT_A_SYNC:
		if(by == 0x00) {
			if( m_currPacketSize > 5) {
				// extra 0, lose one
				m_currPacketSize = 1;
				SendBadPacket("A-Sync ?: Extra 0x00 in sequence");
                memcpy(m_currPacketData,&m_currPacketData[1],5);
				m_curr_packet.type = PKT_A_SYNC;
				// wait for next byte
				m_currPacketSize = 5;
			}
		}
		else if((by == 0x80) && ( m_currPacketSize == 6)) {
			SendPacket();
			m_bStreamSync = true;
		}
		else
		{
            m_currPacketSize--;
            SendBadPacket("A-Sync ? : Unexpected byte in sequence");
			ProcessHeaderByte(by); // send last byte back round...
		}
		break;			
			
	case PKT_CYCLE_COUNT:
		bTopBitSet = ((by & 0x80) == 0x80);
		if(!bTopBitSet || ( m_currPacketSize >= 6))
			SendPacket();				
		break;
			
	case PKT_I_SYNC_CYCLE:
		if(!m_bIsync_got_cycle_cnt) {
			if((by & 0x80) != 0x80) {
				m_bIsync_got_cycle_cnt = true;
			}
			break;
		}
		// fall through when we have the first non-cycle count byte
	case PKT_I_SYNC:
		if(m_bytesExpectedThisPkt == 0) {
			int cycCountBytes = m_currPacketSize - 2;
            int ctxtIDBytes = m_config.CtxtIDBytes();
			// bytes expected = header + n x ctxt id + info byte + 4 x addr; 
            if(m_config.isInstrTrace())
				m_bytesExpectedThisPkt = (short)(cycCountBytes + 6 + ctxtIDBytes);
			else
				m_bytesExpectedThisPkt = (short)(2 + ctxtIDBytes);
			m_IsyncInfoIdx = (short)(1 + cycCountBytes + ctxtIDBytes);
		}
		if(( m_currPacketSize - 1) == m_IsyncInfoIdx) {
			m_bIsync_get_LSiP_addr = ((m_currPacketData[m_IsyncInfoIdx] & 0x80) == 0x80);
		}
			
		// if bytes collected >= bytes expected
		if( m_currPacketSize >= m_bytesExpectedThisPkt) {
			// if we still need the LSip Addr, then this is not part of the expected
			// count as we have no idea how long it is
			if(m_bIsync_get_LSiP_addr) {
				if((by & 0x80) != 0x80) {
					SendPacket();
				}
			}
			else {
				// otherwise, output now
				SendPacket();
			}
		}
		break;
			
	case PKT_NORM_DATA:
		if(m_bExpectingDataAddress && !m_bFoundDataAddress) {
			// look for end of continuation bits
			if((by & 0x80) != 0x80) {
				m_bFoundDataAddress = true;
				// add on the bytes we have found for the address to the expected data bytes
				m_bytesExpectedThisPkt += ( m_currPacketSize - 1);   					
			}
			else 
				break;
		}
		// found any data address we were expecting
		if(m_bytesExpectedThisPkt ==  m_currPacketSize) {
			SendPacket();
		}
		if(m_bytesExpectedThisPkt <  m_currPacketSize)
        {
            SendBadPacket("ERROR : malformed normal data packet");
        }
		break;
			
	case PKT_OOO_DATA:
		if(m_bytesExpectedThisPkt ==  m_currPacketSize)
			SendPacket();
		if(m_bytesExpectedThisPkt <  m_currPacketSize)
			SendBadPacket("ERROR : malformed out of order data packet");
		break;
			
	case PKT_OOO_ADDR_PLC:
		if(m_bExpectingDataAddress) {
			// look for end of continuation bits
			if((by & 0x80) != 0x80) {
				SendPacket();
			}
		}
		break;
			
	case PKT_VAL_NOT_TRACED:
		if(m_bExpectingDataAddress) {
			// look for end of continuation bits
			if((by & 0x80) != 0x80) {
				SendPacket();
			}
		}			
		break;
			
	case PKT_CONTEXT_ID:
		if(m_bytesExpectedThisPkt ==  m_currPacketSize) {
			SendPacket();
		}
		if(m_bytesExpectedThisPkt <  m_currPacketSize)
			SendBadPacket("ERROR : malformed context id packet");
		break;
			
	case PKT_TIMESTAMP:
		if((by & 0x80) != 0x80) {
			SendPacket();
		}
        break;

    case PKT_VMID:
        // single byte payload
        SendPacket();
        break;
	}

}

// extract branch address packet at current location in packet data.
void EtmV3PktProcImpl::OnBranchAddress()
{
    bool exceptionFollows = false;
    int validBits = 0;
    rctdl_vaddr_t partAddr = 0;
  
    partAddr = extractBrAddrPkt(validBits);
    m_curr_packet.UpdateAddress(partAddr,validBits);
}


uint32_t EtmV3PktProcImpl::extractBrAddrPkt(int &nBitsOut)
{
    static int addrshift[] = {
        2, // ARM_ISA
        1, // thumb
        1, // thumb EE
        0  // jazelle
    };

    static uint8_t addrMask[] = {  // byte 5 masks
        0x7, // ARM_ISA
        0xF, // thumb
        0xF, // thumb EE
        0x1F  // jazelle
    };

    static int addrBits[] = { // address bits in byte 5
        3, // ARM_ISA
        4, // thumb
        4, // thumb EE
        5  // jazelle
    };

    static rctdl_armv7_exception exceptionTypeARMdeprecated[] = {
        Excp_Reset,
        Excp_IRQ,
        Excp_Reserved,
        Excp_Reserved,
        Excp_Jazelle,
        Excp_FIQ,
        Excp_AsyncDAbort,
        Excp_DebugHalt
    };

    bool CBit = true;
    int bytecount = 0;
    int bitcount = 0;
    int shift = 0;
    int isa_idx = 0;
    uint32_t value = 0;
    uint8_t addrbyte;
    bool byte5AddrUpdate = false;

    while(CBit && bytecount < 4)
    {
        checkPktLimits();
        addrbyte = m_currPacketData[m_currPktIdx++];
        CBit = (bool)(addrbyte & 0x80);
        shift = bitcount;
        if(bytecount == 0)
        {
            addrbyte &= ~0x1;
            bitcount+=6;
        }
        else
        {
            // bytes 2-4, no continuation, alt format uses bit 6 to indicate following exception bytes
            if(m_config.isAltBranch() && !CBit)
            {
                // last compressed address byte with exception
                if((addrbyte & 0x40) == 0x40)
                    extractExceptionData();
                addrbyte &= 0x3F;
                bitcount+=6;       
            }
            else
            {
                bitcount+=7;
            }
        }
        value |= ((uint32_t)addrbyte) << shift;
        bytecount++;
    }

    // byte 5 - indicates following exception bytes (or not!)
    if(CBit)
    {
        checkPktLimits();
        addrbyte = m_currPacketData[m_currPktIdx++];
        
        // deprecated original byte 5 encoding - ARM state exception only
        if(addrbyte & 0x80)
        {
            uint8_t excep_num = (addrbyte >> 3) & 0x7;
            m_curr_packet.curr_isa = rctdl_isa_arm;
            m_curr_packet.SetException(exceptionTypeARMdeprecated[excep_num], excep_num, (addrbyte & 0x40) ? true : false);
        }
        else
        // normal 5 byte branch, or uses exception bytes.
        {
            // go grab the exception bits to correctly interpret the ISA state
            if((addrbyte & 0x40) == 0x40)
                extractExceptionData();     

            if((addrbyte & 0xB8) == 0x08)
                m_curr_packet.curr_isa = rctdl_isa_arm;
            else if ((addrbyte & 0xB0) == 0x10)
                m_curr_packet.curr_isa = m_curr_packet.bits.curr_alt_isa ? rctdl_isa_tee : rctdl_isa_t16;
            else if ((addrbyte & 0xA0) == 0x20)
                m_curr_packet.curr_isa = rctdl_isa_jazelle;
            else
                /* TBD throw INTRP_ERR_MALFORMED_PACKET */ ;
        }

        byte5AddrUpdate = true; // need to update the address value from byte 5
    }

    // figure out the correct ISA shifts for the address bits
    switch(m_curr_packet.curr_isa) 
    {
    case rctdl_isa_t16: isa_idx = 1; break;
    case rctdl_isa_tee: isa_idx = 2; break;
    case rctdl_isa_jazelle: isa_idx = 3; break;
    default: break;
    }

    if(byte5AddrUpdate)
    {
        value |= ((uint32_t)(addrbyte & addrMask[isa_idx])) << bitcount;
        bitcount += addrBits[isa_idx];
    }
    
    // finally align according to ISA
    shift = addrshift[isa_idx];
    value <<= shift;
    bitcount += shift;

    nBitsOut = bitcount;
    return value;
}

// extract exception data from bytes after address.
void EtmV3PktProcImpl::extractExceptionData()
{
    static const rctdl_armv7_exception exceptionTypesStd[] = {
        Excp_NoException, Excp_DebugHalt, Excp_SMC, Excp_Hyp,
        Excp_AsyncDAbort, Excp_Jazelle, Excp_Reserved, Excp_Reserved,
        Excp_Reset, Excp_Undef, Excp_SVC, Excp_PrefAbort, 
        Excp_SyncDataAbort, Excp_Generic, Excp_IRQ, Excp_FIQ
    };

    static const rctdl_armv7_exception exceptionTypesCM[] = {
        Excp_NoException, Excp_CMIRQn, Excp_CMIRQn, Excp_CMIRQn,
        Excp_CMIRQn, Excp_CMIRQn, Excp_CMIRQn, Excp_CMIRQn, 
        Excp_CMIRQn, Excp_CMUsageFault, Excp_CMNMI, Excp_SVC, 
        Excp_CMDebugMonitor, Excp_CMMemManage, Excp_CMPendSV, Excp_CMSysTick,
        Excp_Reserved, Excp_Reset,  Excp_Reserved, Excp_CMHardFault,
        Excp_Reserved, Excp_CMBusFault, Excp_Reserved, Excp_Reserved
    };

    uint16_t exceptionNum = 0;
    rctdl_armv7_exception excep_type = Excp_Reserved;
    int resume = 0;
    int irq_n = 0;
    bool cancel_prev_instr = 0;
    bool Byte2 = false;

    checkPktLimits();

    //**** exception info Byte 0
    uint8_t dataByte =  m_currPacketData[m_currPktIdx++];

    m_curr_packet.bits.curr_NS = dataByte & 0x1;
    exceptionNum |= (dataByte >> 1) & 0xF;
    cancel_prev_instr = (dataByte & 0x20) ? true : false;
    m_curr_packet.bits.curr_alt_isa = ((dataByte & 0x40) != 0)  ? 1 : 0; 

    //** another byte?
    if(dataByte & 0x80)
    {
        checkPktLimits();
        dataByte = m_currPacketData[m_currPktIdx++];

        if(dataByte & 0x40)
            Byte2 = true;   //** immediate info byte 2, skipping 1
        else
        {
            //**** exception info Byte 1
            if(m_config.isV7MArch())
            {
                exceptionNum |= ((uint16_t)(dataByte & 0x1F)) << 4;
            }
             m_curr_packet.bits.curr_Hyp = dataByte & 0x20 ? 1 : 0;

            if(dataByte & 0x80)
            {
                checkPktLimits();
                dataByte = m_currPacketData[m_currPktIdx++];
                Byte2 = true;
            }
        }
        //**** exception info Byte 2
        if(Byte2)
        {
            resume = dataByte & 0xF;
        }
    }

    // set the exception type - according to the number and core profile
    if(m_config.isV7MArch())
    {
       exceptionNum &= 0x1FF;
        if(exceptionNum < 0x018)
            excep_type= exceptionTypesCM[exceptionNum];
        else
            excep_type = Excp_CMIRQn;

        if(excep_type == Excp_CMIRQn)
        {
            if(exceptionNum > 0x018)
                irq_n = exceptionNum - 0x10;
            else if(exceptionNum == 0x008)
                irq_n = 0;
            else
                irq_n = exceptionNum;
        }
    }
    else
    {
        exceptionNum &= 0xF;
        excep_type = exceptionTypesStd[exceptionNum];
    }
    m_curr_packet.SetException(excep_type, exceptionNum, cancel_prev_instr,irq_n,resume);
}

void EtmV3PktProcImpl::checkPktLimits()
{
    // index running off the end of the packet means a malformed packet.
    if(m_currPktIdx >= m_currPacketData.size())
        /*TBD : throw a suitable error here */ ;
}

/* End of File trc_pkt_proc_etmv3_impl.cpp */

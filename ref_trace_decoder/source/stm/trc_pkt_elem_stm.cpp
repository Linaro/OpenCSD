/*
 * \file       trc_pkt_elem_stm.cpp
 * \brief      Reference CoreSight Trace Decoder : STM decode - packet class
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

#include <sstream>
#include <iomanip>
#include "stm/trc_pkt_elem_stm.h"

StmTrcPacket::StmTrcPacket()
{
    initStartState();
}

StmTrcPacket &StmTrcPacket::operator =(const rctdl_stm_pkt *p_pkt)
{
    *dynamic_cast<rctdl_stm_pkt *>(this) = *p_pkt;
    return *this;
}

void StmTrcPacket::initStartState()
{
    master = 0;
    channel = 0;
    timestamp = 0;
    ts_type = STM_TS_UNKNOWN; 
    type =  STM_PKT_NOTSYNC;       
    initNextPacket();
}

void StmTrcPacket::initNextPacket()
{
    err_type = STM_PKT_NO_ERR_TYPE;
    pkt_ts_bits = 0;
    pkt_has_marker = 0;
}

void StmTrcPacket::setTS(const uint64_t ts_val, const uint8_t updatedBits)
{
    if(updatedBits == 64)
    {
        timestamp = ts_val;
    }
    else
    {
        uint64_t mask = (0x1ULL << updatedBits) - 1;
        timestamp &= ~mask;
        timestamp |= ts_val & mask;
    }
    pkt_ts_bits = updatedBits;  // mark number of bits 
}

// printing
void StmTrcPacket::toString(std::string &str) const
{
    std::string name, desc;
    std::ostringstream oss;

    name = pktTypeName(type,desc);
    str = name + ":" + desc;

    // extended information
    switch(type)
    {
    case STM_PKT_INCOMPLETE_EOT:
    case STM_PKT_BAD_SEQUENCE:
        name = pktTypeName(err_type,desc);
        str+= "[" + name + "]";
        break;

    case STM_PKT_VERSION:
        oss << "; Ver=" << payload.D8;
        str+= oss.str();
        break;

    case STM_PKT_FREQ:
        oss << "; Freq=" << std::dec << payload.D32 << "Hz";
        str+= oss.str();
        break;

    case STM_PKT_TRIG:
        oss << "; TrigData=0x" << std::hex << std::setw(2) << std::setfill('0') << payload.D8;
        str+= oss.str();
        break;

    case STM_PKT_M8:
        oss << "; Master=" << std::dec << master;
        str+= oss.str();
        break;
       
    case STM_PKT_C8:
    case STM_PKT_C16:
        oss << "; Chan=0x" << std::hex << std::setw(4) << std::setfill('0') << channel;
        str+= oss.str();
        break;

    case STM_PKT_D8: 
        oss << "; Data=0x" << std::hex << std::setw(2) << std::setfill('0') << payload.D8;
        str+= oss.str();
        break;

    case STM_PKT_D16:
        oss << "; Data=0x" << std::hex << std::setw(4) << std::setfill('0') << payload.D16;
        str+= oss.str();
        break;

    case STM_PKT_D32:
        oss << "; Data=0x" << std::hex << std::setw(8) << std::setfill('0') << payload.D32;
        str+= oss.str();
        break;

    case STM_PKT_D64:
        oss << "; Data=0x" << std::hex << std::setw(16) << std::setfill('0') << payload.D64;
        str+= oss.str();
        break;
    }

    if(isTSPkt())
    {
        std::string valStr;
        trcPrintableElem::getValStr(valStr,64,64,timestamp,true,pkt_ts_bits);
        str += "; TS=" + valStr;
    }
}

void StmTrcPacket::toStringFmt(const uint32_t fmtFlags, std::string &str) const
{
    // no formatting for now.
    toString(str);
}

std::string &StmTrcPacket::pktTypeName(const rctdl_stm_pkt_type pkt_type, std::string &desc) const
{
    std::ostringstream oss_name;
    std::ostringstream oss_desc;
    bool addMarkerTS = false;

    oss_name.str("UNKNOWN");
    oss_desc.str("ERROR: unknown packet type");

    switch(pkt_type)
    {
    case STM_PKT_RESERVED: 
        oss_name.str("RESERVED");
        oss_desc.str("Reserved Packet Header");
        break;

    case STM_PKT_NOTSYNC:
        oss_name.str("NOTSYNC");
        oss_desc.str("STM not synchronised");
        break;

    case STM_PKT_INCOMPLETE_EOT:
        oss_name.str("INCOMPLETE_EOT");
        oss_desc.str("Incomplete packet flushed at end of trace");
        break;

    case STM_PKT_NO_ERR_TYPE:
        oss_name.str("NO_ERR_TYPE");
        oss_desc.str("Error type not set");
        break;

    case STM_PKT_BAD_SEQUENCE:
        oss_name.str("BAD_SEQUENCE");
        oss_desc.str("Invalid sequence in packet");
        break;

    case STM_PKT_ASYNC:
        oss_name.str("ASYNC");
        oss_desc.str("Alignment synchronisation packet");
        break;

    case STM_PKT_VERSION:
        oss_name.str("VERSION");
        oss_desc.str("Version packet");
        break;

    case STM_PKT_FREQ:
        oss_name.str("FREQ");
        oss_desc.str("Frequency packet");
        break;

    case STM_PKT_NULL:
        oss_name.str("NULL");
        oss_desc.str("Null packet");
        break;

    case STM_PKT_TRIG:
        oss_name.str("TRIG");
        oss_desc.str("Trigger packet");
        addMarkerTS = true;
        break;

    case STM_PKT_GERR:
        oss_name.str("GERR");
        oss_desc.str("Global Error");
        break;

    case STM_PKT_MERR:
        oss_name.str("MERR");
        oss_desc.str("Master Error");
        break;

    case STM_PKT_M8:
        oss_name.str("M8");
        oss_desc.str("Set current master");
        break;

    case STM_PKT_C8:
        oss_name.str("C8");
        oss_desc.str("Set current channel");
        break;

    case STM_PKT_C16:
        oss_name.str("C16");
        oss_desc.str("Set current channel");
        break;

    case STM_PKT_FLAG:
        oss_name.str("FLAG");
        oss_desc.str("Flag packet");
        addMarkerTS = true;
        break;

    case STM_PKT_D8:
        oss_name.str("D8");
        oss_desc.str("8 bit data");
        addMarkerTS = true;
        break;

    case STM_PKT_D16:
        oss_name.str("D16");
        oss_desc.str("16 bit data");
        addMarkerTS = true;
        break;

    case STM_PKT_D32:
        oss_name.str("D32");
        oss_desc.str("32 bit data");
        addMarkerTS = true;
        break;

    case STM_PKT_D64:
        oss_name.str("D64");
        oss_desc.str("64 bit data");
        addMarkerTS = true;
        break;

    default:
        break;
    }

    if(addMarkerTS)
    {
        if(isMarkerPkt())
        {
            oss_name << "M";
            oss_desc << "; with marker";
        }

        if(isTSPkt())
        {
            oss_name << "TS";
            oss_desc << "; with timestamp";
        }
    }
    oss_desc << ".";
    desc = oss_desc.str();
    return oss_name.str();
}


/* End of File trc_pkt_elem_stm.cpp */

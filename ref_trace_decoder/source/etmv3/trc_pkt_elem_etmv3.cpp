/*
 * \file       trc_pkt_elem_etmv3.cpp
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

#include <sstream>
#include <iomanip>

#include "etmv3/trc_pkt_elem_etmv3.h"

EtmV3TrcPacket::EtmV3TrcPacket()
{
    addr.size = VA_32BIT;   // etm v3 only handles 32 bit addresses.
}

EtmV3TrcPacket::~EtmV3TrcPacket()
{
}

// update interface - set packet values

// clear this packet info
void EtmV3TrcPacket::Clear()    
{
    addr.pkt_bits = 0;
    prev_isa = curr_isa;    // mark ISA as not changed
    exception.bits.present = 0;
    atom.num = 0;
    cycle_count = 0;
    context.updated = 0;
    context.updated_c = 0;
    context.updated_v = 0;
    data.ooo_tag = 0;
    data.value = 0;
    data.update_addr = 0;
    data.update_be = 0;
    data.update_dval = 0;
    ts_update_bits = 0;
    isync_info.has_cycle_count = 0;
    isync_info.has_LSipAddress = 0;
    isync_info.no_address = 0;
}

// reset all state including intra packet
void EtmV3TrcPacket::ResetState()   
{
    addr.val = 0;
    addr.valid_bits = 0;
    curr_isa = prev_isa = rctdl_isa_unknown;
    context.curr_alt_isa = 0;
    context.curr_NS = 0;     
    context.curr_Hyp = 0;  
    context.VMID = 0;
    context.ctxtID = 0;    
    timestamp = 0;
    data.addr.valid_bits = 0;
    data.addr.val = 0;
    data.be = 0;
    Clear();
}

void EtmV3TrcPacket::UpdateAddress(const rctdl_vaddr_t partAddrVal, const int updateBits)
{
    rctdl_vaddr_t validMask = RCTDL_VA_MASK;
    validMask >>= RCTDL_MAX_VA_BITSIZE-updateBits;
    addr.pkt_bits = updateBits;
    addr.val &= ~validMask;
    addr.val |= (partAddrVal & validMask);
    if(updateBits > addr.valid_bits)
        addr.valid_bits = updateBits;    
}

void EtmV3TrcPacket::UpdateDataAddress(const uint32_t value, const uint8_t valid_bits)
{
    // ETMv3 data addresses 32 bits.
    uint32_t validMask = 0xFFFFFFFF;
    validMask >>= 32-valid_bits;
    addr.pkt_bits = valid_bits;
    addr.val &= ~validMask;
    addr.val |= (value & validMask);
    if(valid_bits > addr.valid_bits)
        addr.valid_bits = valid_bits;
    data.update_addr = 1;
}

void EtmV3TrcPacket::UpdateTimestamp(const uint64_t tsVal, const uint8_t updateBits)
{
    uint64_t validMask = ~0ULL;
    validMask >>= 64-updateBits;
    timestamp &= ~validMask;
    timestamp |= (tsVal & validMask);
    ts_update_bits = updateBits;
}



void EtmV3TrcPacket::SetException(  const rctdl_armv7_exception type, 
                                    const uint16_t number, 
                                    const bool cancel,
                                    const bool cm_type,
                                    const int irq_n  /*= 0*/,
                                    const int resume /* = 0*/)
{
    // initial data
    exception.bits.cancel = cancel ? 1 : 0;
    exception.bits.cm_irq_n = irq_n;
    exception.bits.cm_resume = resume;
    exception.bits.cm_type = cm_type ? 1 : 0;
    exception.number = number;
    exception.type = type;

    // mark as valid in this packet
    exception.bits.present = 1;
}

bool EtmV3TrcPacket::UpdateAtomFromPHdr(const uint8_t pHdr, const bool cycleAccurate)
{
    bool bValid = true;
    uint8_t E = 0, N = 0;
    if(!cycleAccurate)
    {
        if((pHdr & 0x3) == 0x0)
        {
            E = ((pHdr >> 2) & 0xF);
            N = (pHdr & 0x40) ? 1 : 0;
            atom.num = E+N;
            atom.En_bits = (((uint32_t)0x1) << E) - 1;
            p_hdr_fmt = 1;
        }
        else if((pHdr & 0x3) == 0x2)
        {
            atom.num = 2;
            p_hdr_fmt = 2;
            atom.En_bits = (pHdr & 0x8 ? 0 : 1) |  (pHdr & 0x4 ? 0 : 0x2);
        }
        else 
            bValid = false;
    }
    else
    {
        uint8_t pHdr_code = pHdr & 0xA3;
        switch(pHdr_code)
        {
        case 0x80:
            p_hdr_fmt = 1;
            E = ((pHdr >> 2) & 0x7);
            N = (pHdr & 0x40) ? 1 : 0;
            atom.num = E+N;
            if(atom.num)
            {
                atom.En_bits = (((uint32_t)0x1) << E) - 1;
                cycle_count = E+N;
            }
            else 
                bValid = false; // deprecated 8b'10000000 code

            break;

        case 0x82:
            p_hdr_fmt = 2;
            if(pHdr & 0x10)
            {
                p_hdr_fmt = 4;
                atom.num = 1;
                cycle_count  = 0;
                atom.En_bits = pHdr & 0x04 ? 0 : 1;
            }
            else
            {
                atom.num = 2;
                cycle_count  = 1;
                atom.En_bits = (pHdr & 0x8 ? 0 : 1) |  (pHdr & 0x4 ? 0 : 0x2);
            }
            break;

        case 0xA0:
            p_hdr_fmt = 3;
            cycle_count  = ((pHdr >> 2) & 7) + 1;
            E = pHdr & 0x40 ? 1 : 0;
            atom.num = E;
            atom.En_bits = E;
            break;

        default:
            bValid = false;
            break;

        }
    }
    return bValid;
}

EtmV3TrcPacket &EtmV3TrcPacket::operator =(const rctdl_etmv3_pkt* p_pkt)
{
    *dynamic_cast<rctdl_etmv3_pkt *>(this) = *p_pkt;
    return *this;        
}

    // printing
void EtmV3TrcPacket::toString(std::string &str) const
{
    const char *name;
    const char *desc;
    std::string valStr, ctxtStr = "";

    name = packetTypeName(type, &desc);
    str = name + (std::string)" : " + desc;
    
    switch(type)
    {
        // print the original header type for the bad sequences.
    case ETM3_PKT_BAD_SEQUENCE: 
    case ETM3_PKT_BAD_TRACEMODE:
        name = packetTypeName(err_type,0);
        str += "[" + (std::string)name + "]";
        break;

    case ETM3_PKT_BRANCH_ADDRESS:
        getBranchAddressStr(valStr);
        str += "; " + valStr;
        break;

    case ETM3_PKT_I_SYNC_CYCLE:
    case ETM3_PKT_I_SYNC:
        getISyncStr(valStr);
        str += "; " + valStr;
        break;

    case ETM3_PKT_P_HDR:
        getAtomStr(valStr);
        str += "; " + valStr;
        break;

    case ETM3_PKT_CYCLE_COUNT:
        {
            std::ostringstream oss;
            oss << "; Cycles=" << cycle_count;
            str += oss.str();
        }
        break;

    case ETM3_PKT_CONTEXT_ID:
        {
            std::ostringstream oss;
            oss << "; CtxtID=" << std::hex << "0x" << context.ctxtID;
            str += oss.str();
        }
        break;

    case ETM3_PKT_VMID:
        {
            std::ostringstream oss;
            oss << "; VMID=" << std::hex << "0x" << context.VMID;
            str += oss.str();
        }
        break;

    case ETM3_PKT_TIMESTAMP:
        {
            std::ostringstream oss;
            oss << "; TS=" << std::hex << "0x" << timestamp << " (" << std::dec << timestamp << ") ";
            str += oss.str();
        }
        break;

    case ETM3_PKT_OOO_DATA:
        {
            std::ostringstream oss;
            oss << "; Val=" << std::hex << "0x" << data.value;
            oss << "; OO_Tag=" << std::hex << "0x" << data.ooo_tag;
            str += oss.str();
        }
        break;

    case ETM3_PKT_VAL_NOT_TRACED:
        if(data.update_addr)
        {
            trcPrintableElem::getValStr(valStr,32, data.addr.valid_bits, data.addr.val,true,data.addr.pkt_bits);
            str += "; Addr=" + valStr;
        }
        break;

    case ETM3_PKT_OOO_ADDR_PLC:
        if(data.update_addr)
        {
            trcPrintableElem::getValStr(valStr,32, data.addr.valid_bits, data.addr.val,true,data.addr.pkt_bits);
            str += "; Addr=" + valStr;
        }
        {
            std::ostringstream oss;
            oss << "; OO_Tag=" << std::hex << "0x" << data.ooo_tag;
            str += oss.str();
        }
        break;

    case ETM3_PKT_NORM_DATA:
        if(data.update_addr)
        {
            trcPrintableElem::getValStr(valStr,32, data.addr.valid_bits, data.addr.val,true,data.addr.pkt_bits);
            str += "; Addr=" + valStr;
        }
        if(data.update_dval)
        {
            std::ostringstream oss;
            oss << "; Val=" << std::hex << "0x" << data.value;
            str += oss.str();
        }
        break;
    }
}

void EtmV3TrcPacket::toStringFmt(const uint32_t fmtFlags, std::string &str) const
{
    // no formatting implemented at present.
    toString(str);
}

const char *EtmV3TrcPacket::packetTypeName(const rctdl_etmv3_pkt_type type, const char **ppDesc) const
{
    const char *pName = "I_RESERVED";
    const char *pDesc = "Reserved Packet Header";

    switch(type)
    {
// markers for unknown packets
    // case ETM3_PKT_NOERROR:,        //!< no error in packet - supplimentary data.
    case ETM3_PKT_NOTSYNC:        //!< no sync found yet
        pName = "NOTSYNC";
        pDesc = "Trace Stream not synchronised";
        break;

    case ETM3_PKT_INCOMPLETE_EOT: //!< flushing incomplete/empty packet at end of trace.
        pName = "INCOMPLETE_EOT.";
        pDesc = "Incomplete packet at end of trace data.";
        break;

// markers for valid packets
    case ETM3_PKT_BRANCH_ADDRESS:
        pName = "BRANCH_ADDRESS";
        pDesc = "Branch address.";
        break;

    case ETM3_PKT_A_SYNC:
        pName = "A_SYNC";
        pDesc = "Alignment Synchronisation.";
        break;

    case ETM3_PKT_CYCLE_COUNT:
        pName = "CYCLE_COUNT";
        pDesc = "Cycle Count.";
        break;

    case ETM3_PKT_I_SYNC:
        pName = "I_SYNC";
        pDesc = "Instruction Packet synchronisation.";
        break;

    case ETM3_PKT_I_SYNC_CYCLE:
        pName = "I_SYNC_CYCLE";
        pDesc = "Instruction Packet synchronisation with cycle count.";
        break;

    case ETM3_PKT_TRIGGER:
        pName = "TRIGGER";
        pDesc = "Trace Trigger Event.";
        break;

    case ETM3_PKT_P_HDR:
        pName = "P_HDR";
        pDesc = "Atom P-header.";
        break;

    case ETM3_PKT_STORE_FAIL:
        pName = "STORE_FAIL";
        pDesc = "Data Store Failed.";
        break; 

    case ETM3_PKT_OOO_DATA:
        pName = "OOO_DATA";
        pDesc = "Out of Order data value packet.";
        break;

    case ETM3_PKT_OOO_ADDR_PLC:
        pName = "OOO_ADDR_PLC";
        pDesc = "Out of Order data address placeholder.";
        break;

    case ETM3_PKT_NORM_DATA:
        pName = "NORM_DATA";
        pDesc = "Data trace packet.";
        break;

    case ETM3_PKT_DATA_SUPPRESSED:
        pName = "DATA_SUPPRESSED";
        pDesc = "Data trace suppressed.";
        break;

    case ETM3_PKT_VAL_NOT_TRACED:
        pName = "VAL_NOT_TRACED";
        pDesc = "Data trace value not traced.";
        break;

    case ETM3_PKT_IGNORE:
        pName = "IGNORE";
        pDesc = "Packet ignored.";
        break;

    case ETM3_PKT_CONTEXT_ID:
        pName = "CONTEXT_ID";
        pDesc = "Context ID change.";
        break;

    case ETM3_PKT_VMID:
        pName = "VMID";
        pDesc = "VMID change.";
        break;

    case ETM3_PKT_EXCEPTION_ENTRY:
        pName = "EXCEPTION_ENTRY";
        pDesc = "Exception entry data marker.";
        break;

    case ETM3_PKT_EXCEPTION_EXIT:
        pName = "EXCEPTION_EXIT";
        pDesc = "Exception return.";
        break;

    case ETM3_PKT_TIMESTAMP:
        pName = "TIMESTAMP";
        pDesc = "Timestamp Value.";
        break;

// internal processing types
    // case ETM3_PKT_BRANCH_OR_BYPASS_EOT: not externalised

// packet errors 
    case ETM3_PKT_BAD_SEQUENCE: 
        pName = "BAD_SEQUENCE";
        pDesc = "Invalid sequence for packet type.";
        break;

    case ETM3_PKT_BAD_TRACEMODE:  
        pName = "BAD_TRACEMODE";
        pDesc = "Invalid packet type for this trace mode.";
        break;

        // leave thest unchanged.
    case ETM3_PKT_RESERVED:
    default:
        break;

    }

    if(ppDesc) *ppDesc = pDesc;
    return pName;
}

void EtmV3TrcPacket::getBranchAddressStr(std::string &valStr) const
{
    std::ostringstream oss;
    std::string subStr;

    // print address.
    trcPrintableElem::getValStr(subStr,32,addr.valid_bits,addr.val,true,addr.pkt_bits);
    oss << "Addr=" << subStr << "; ";

    // current ISA if changed.
    if(curr_isa != prev_isa)
    {
        getISAStr(subStr);
        oss << subStr;
    }

    // S / NS etc if changed.
    if(context.updated)
    {
        oss << context.curr_NS ? "NS; " : "S; ";
        oss << context.curr_Hyp ? "Hyp; " : "";
    }
    
    // exception? 
    if(exception.bits.present)
    {
        getExcepStr(subStr);
        oss << subStr;
    }
    valStr = oss.str();
}

void EtmV3TrcPacket::getAtomStr(std::string &valStr) const
{   
    std::ostringstream oss;
    uint32_t bitpattern = atom.En_bits; // arranged LSBit oldest, MSbit newest

    if(!cycle_count)
    {
        for(int i = 0; i < atom.num; i++)
        {
            oss << ((bitpattern & 0x1) ? "E" : "N"); // in spec read L->R, oldest->newest
            bitpattern >>= 1;
        }        
    }
    else
    {
        switch(p_hdr_fmt)
        {
        case 1:
            for(int i = 0; i < atom.num; i++)
            {
                oss << ((bitpattern & 0x1) ? "WE" : "WN"); // in spec read L->R, oldest->newest
                bitpattern >>= 1;
            }        
            break;

        case 2:
            oss << "W";
            for(int i = 0; i < atom.num; i++)
            {
                oss << ((bitpattern & 0x1) ? "E" : "N"); // in spec read L->R, oldest->newest
                bitpattern >>= 1;
            }        
            break;

        case 3:
            for(uint32_t i = 0; i < cycle_count; i++)
                oss << "W";
            if(atom.num)
                oss << ((bitpattern & 0x1) ? "E" : "N"); // in spec read L->R, oldest->newest
            break;
        }
        oss << "; Cycles=" << cycle_count;
    }
    valStr = oss.str();
}

void EtmV3TrcPacket::getISyncStr(std::string &valStr) const
{
    std::ostringstream oss;
    static const char *reason[] = { "Periodic", "Trace Enable", "Restart Overflow", "Debug Exit" };

    // reason.
    oss << "(" << reason[(int)isync_info.reason] << "); ";

    // full address.
    if(!isync_info.no_address)
    {
        if(isync_info.has_LSipAddress)
            oss << "Data Instr Addr=0x";
        else
            oss << "Addr=0x";
        oss << std::hex << std::setfill('0') << std::setw(8) << addr.val << "; ";
    }
    
    oss << (context.curr_NS ? "NS; " : "S; ");
    oss << (context.curr_Hyp ? "Hyp; " : " ");
    
    if(context.updated_c)
    {
        oss << "CtxtID=" << std::hex << context.ctxtID << "; ";
    }

    if(isync_info.no_address)
    {
        valStr = oss.str();
        return;     // bail out at this point if a data only ISYNC
    }

    std::string isaStr;
    getISAStr(isaStr);
    oss << isaStr;

    if(isync_info.has_cycle_count)
    {
        oss << "Cycles=" << std::dec << cycle_count << "; ";
    }

    if(isync_info.has_LSipAddress)
    {
        std::string addrStr;

        // extract address updata.
        trcPrintableElem::getValStr(addrStr,32,data.addr.valid_bits,data.addr.val,true,data.addr.pkt_bits);
        oss << "Curr Instr Addr=" << addrStr << ";";        
    }    
    valStr = oss.str();
}

void EtmV3TrcPacket::getISAStr(std::string &isaStr) const
{
    std::ostringstream oss;
    oss << "ISA=";
    switch(curr_isa)
    {
    case rctdl_isa_arm: 
        oss << "ARM(32); ";
        break;

    case rctdl_isa_thumb2:
        oss << "Thumb2; ";
        break;

    case rctdl_isa_aarch64:
        oss << "AArch64; ";
        break;

    case rctdl_isa_tee:
        oss << "ThumbEE; ";
        break;

    case rctdl_isa_jazelle:
        oss << "Jazelle; ";
        break;

    default:
    case rctdl_isa_unknown:
        oss << "Unknown; ";
        break;
    }
    isaStr = oss.str();
}

void EtmV3TrcPacket::getExcepStr(std::string &excepStr) const
{
    static const char *ARv7Excep[] = {
        "No Exception", "Debug Halt", "SMC", "Hyp", 
        "Async Data Abort", "Jazelle", "Reserved", "Reserved",
        "PE Reset", "Undefined Instr", "SVC", "Prefetch Abort", 
        "Data Fault", "Generic", "IRQ", "FIQ"
    };

    static const char *MExcep[] = {
        "No Exception", "IRQ1", "IRQ2", "IRQ3",
        "IRQ4", "IRQ5", "IRQ6", "IRQ7",
        "IRQ0","usage Fault","NMI","SVC",
        "DebugMonitor", "Mem Manage","PendSV","SysTick",
        "Reserved","PE Reset","Reserved","HardFault"
        "Reserved","BusFault","Reserved","Reserved"
    };

    std::ostringstream oss;
    oss << "Exception=";

    if(exception.bits.cm_type)
    {
        if(exception.number < 0x18)
            oss << MExcep[exception.number];
        else
            oss << "IRQ" << std::dec << (exception.number - 0x10);
        if(exception.bits.cm_resume)
            oss << "; Resume=" << exception.bits.cm_resume;
        if(exception.bits.cancel)
            oss << "; Cancel prev instr";
    }
    else
    {
        oss << ARv7Excep[exception.number] << "; ";
        if(exception.bits.cancel)
            oss << "; Cancel prev instr";
    }
    excepStr = oss.str();
}
/* End of File trc_pkt_elem_etmv3.cpp */

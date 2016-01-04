/*
 * \file       trc_pkt_elem_ptm.cpp
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
#include "ptm/trc_pkt_elem_ptm.h"

PtmTrcPacket::PtmTrcPacket()
{
}

PtmTrcPacket::~PtmTrcPacket()
{
}

void PtmTrcPacket::Clear()
{
    err_type = PTM_PKT_NOERROR;
    cycle_count = 0;
    context.updated = 0;
    context.updated_c = 0;
    context.updated_v = 0;
    ts_update_bits = 0;
    atom.En_bits = 0;
    exception.bits.present = 0;
}

void PtmTrcPacket::ResetState()
{
    type = PTM_PKT_NOTSYNC;

    context.ctxtID = 0;
    context.VMID = 0;
    context.curr_alt_isa = 0;
    context.curr_Hyp = 0;
    context.curr_NS = 0;

    addr.valid_bits = 0;
    addr.size = VA_32BIT;

    prev_isa = curr_isa = rctdl_isa_unknown;

    timestamp = 0; 

    Clear();
}

void PtmTrcPacket::UpdateAddress(const rctdl_vaddr_t partAddrVal, const int updateBits)
{
    rctdl_vaddr_t validMask = RCTDL_VA_MASK;
    validMask >>= RCTDL_MAX_VA_BITSIZE-updateBits;
    addr.pkt_bits = updateBits;
    addr.val &= ~validMask;
    addr.val |= (partAddrVal & validMask);
    if(updateBits > addr.valid_bits)
        addr.valid_bits = updateBits;    
}

void PtmTrcPacket::UpdateTimestamp(const uint64_t tsVal, const uint8_t updateBits)
{
    uint64_t validMask = ~0ULL;
    validMask >>= 64-updateBits;
    timestamp &= ~validMask;
    timestamp |= (tsVal & validMask);
    ts_update_bits = updateBits;
}
   
bool PtmTrcPacket::UpdateAtomFromPHdr(const uint8_t pHdr, const bool cycleAccurate, const uint32_t cycleCount)
{
    if(cycleAccurate)
    {
        cycle_count = cycleCount;
        atom.num = 1;
        atom.En_bits = (pHdr & 0x2) ? 0x0 : 0x1;
    }
    else
    {
        // how many atoms
        uint8_t atom_fmt_id = pHdr & 0xF0;
        if(atom_fmt_id == 0x80)
        {
            // format 1 or 2
            if((pHdr & 0x08) == 0x08)
                atom.num = 2;
            else
                atom.num = 1;
        }
        else if(atom_fmt_id == 0x90)
        {
            atom.num = 3;
        }
        else
        {
            if((pHdr & 0xE0) == 0xA0)
                atom.num = 4;
            else
                atom.num = 5;
        }
        
        // extract the E/N bits
        uint8_t atom_mask = 0x2;    // start @ bit 1 - newest instruction 
        atom.En_bits = 0;
        for(int i = 0; i < atom.num; i++)
        {
            atom.En_bits <<= 1;
            if(!(atom_mask & pHdr)) // 0 bit is an E in PTM -> a one in the standard atom bit type
                atom.En_bits |= 0x1;
            atom_mask <<= 1;
        }
        
    }
}

    // printing
void PtmTrcPacket::toString(std::string &str) const
{
}

void PtmTrcPacket::toStringFmt(const uint32_t fmtFlags, std::string &str) const
{
}

/* End of File trc_pkt_elem_ptm.cpp */

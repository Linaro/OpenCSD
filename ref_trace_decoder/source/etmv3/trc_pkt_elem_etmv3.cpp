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
}

// reset all state including intra packet
void EtmV3TrcPacket::ResetState()   
{
    addr.val = 0;
    addr.valid_bits = 0;
    curr_isa = prev_isa = rctdl_isa_unknown;
    bits.curr_alt_isa = 0;
    bits.curr_NS = 0;     
    bits.curr_Hyp = 0;  
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

void EtmV3TrcPacket::SetException(  const rctdl_armv7_exception type, 
                                    const uint16_t number, 
                                    const bool cancel,
                                    const int irq_n  /*= 0*/,
                                    const int resume /* = 0*/)
{
    // initial data
    exception.bits.cancel = cancel ? 1 : 0;
    exception.bits.cm_irq_n = irq_n;
    exception.bits.cm_resume = resume;
    exception.number = number;
    exception.type = type;

    // mark as valid in this packet
    exception.bits.present = 1;
}

    // printing
void EtmV3TrcPacket::toString(std::string &str) const
{
}

void EtmV3TrcPacket::toStringFmt(const uint32_t fmtFlags, std::string &str) const
{
}



/* End of File trc_pkt_elem_etmv3.cpp */

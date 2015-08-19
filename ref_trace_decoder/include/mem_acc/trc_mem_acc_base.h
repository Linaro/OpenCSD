/*
 * \file       trc_mem_acc_base.h
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


#ifndef ARM_TRC_MEM_ACC_BASE_H_INCLUDED
#define ARM_TRC_MEM_ACC_BASE_H_INCLUDED

#include "rctdl_if_types.h"

class TrcMemAccessorBase
{
public:
    TrcMemAccessorBase();
    TrcMemAccessorBase(rctdl_vaddr_t startAddr, rctdl_vaddr_t endAddr);
    virtual ~TrcMemAccessorBase() {};

    void setRange(rctdl_vaddr_t startAddr, rctdl_vaddr_t endAddr);

    const bool addrInRange(const rctdl_vaddr_t s_address) const;
    const uint32_t bytesInRange(const rctdl_vaddr_t s_address, const uint32_t reqBytes) const;

    virtual const uint32_t readBytes(const rctdl_vaddr_t s_address, const uint32_t reqBytes, uint8_t *byteBuffer) = 0;

protected:
    rctdl_vaddr_t m_startAddress;
    rctdl_vaddr_t m_endAddress;
};

inline TrcMemAccessorBase::TrcMemAccessorBase(rctdl_vaddr_t startAddr, rctdl_vaddr_t endAddr) :
     m_startAddress(startAddr),
     m_endAddress(endAddr)
{
}

inline TrcMemAccessorBase::TrcMemAccessorBase() :
     m_startAddress(0),
     m_endAddress(0)
{
}

inline void TrcMemAccessorBase::setRange(rctdl_vaddr_t startAddr, rctdl_vaddr_t endAddr)
{
     m_startAddress = startAddr;
     m_endAddress = endAddr;
}

inline const bool TrcMemAccessorBase::addrInRange(const rctdl_vaddr_t s_address) const
{
    return (s_address >= m_startAddress) && (s_address <= m_endAddress);
}

inline const uint32_t TrcMemAccessorBase::bytesInRange(const rctdl_vaddr_t s_address, const uint32_t reqBytes) const
{
    uint32_t bytesInRange = 0;
    if(addrInRange(s_address))
    {
        if(addrInRange(s_address+reqBytes-1))
            bytesInRange = reqBytes;
        else
            bytesInRange = reqBytes - (s_address + reqBytes - m_endAddress);
    }
    return bytesInRange;
}

#endif // ARM_TRC_MEM_ACC_BASE_H_INCLUDED

/* End of File trc_mem_acc_base.h */

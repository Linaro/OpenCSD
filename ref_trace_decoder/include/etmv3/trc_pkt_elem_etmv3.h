/*
 * \file       trc_pkt_elem_etmv3.h
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

#ifndef ARM_TRC_PKT_ELEM_ETMV3_H_INCLUDED
#define ARM_TRC_PKT_ELEM_ETMV3_H_INCLUDED

#include "trc_pkt_types_etmv3.h"
#include "trc_printable_elem.h"

/** @addtogroup trc_pkts
@{*/

/*!
 * @class EtmV3TrcPacket   
 * @brief ETMv3 Trace Protocol Packet.
 * 
 *  This class represents a single ETMv3 trace packet, along with intra packet state.
 * 
 */
class EtmV3TrcPacket : public rctdl_etmv3_pkt, trcPrintableElem
{
public:
    EtmV3TrcPacket();
    ~EtmV3TrcPacket();

    // update interface - set packet values
    void Clear();       //!< clear update data in packet ready for new one.
    void ResetState();  //!< reset intra packet state data

    void SetType(const rctdl_etmv3_pkt_type p_type);
    void SetErrType(const rctdl_etmv3_pkt_type e_type);
    void UpdateAddress(const rctdl_vaddr_t partAddrVal, const int updateBits);
    void SetException(  const rctdl_armv7_exception type, 
                        const uint16_t number, 
                        const bool cancel,
                        const int irq_n = 0,
                        const int resume = 0);
    void UpdateNS(const int NS);
    void UpdateAltISA(const int AltISA);
    void UpdateHyp(const int Hyp);
    void UpdateISA(const rctdl_isa isa);
    void UpdateContextID(const uint32_t contextID);
    void UpdateVMID(const uint8_t VMID);
    void UpdateTimestamp(const uint64_t tsVal, const uint8_t updateBits);
    
    bool UpdateAtomFromPHdr(const uint8_t pHdr, const bool cycleAccurate);  //!< Interpret P Hdr, return true if valid, false if not.

    void SetDataOOOTag(const uint8_t tag);
    void SetDataValue(const uint32_t value);
    void UpdateDataAddress(const uint32_t value, const uint8_t valid_bits);
    void UpdateDataEndian(const uint8_t BE_Val);
    void SetCycleCount(const uint32_t cycleCount);
    void SetISyncReason(const etmv3_isync_reason_t reason);
    void SetISyncHasCC();
    void SetISyncIsLSiP();

    // packet status interface - get packet info.
    const int AltISA() const { return context.curr_alt_isa; };
    const rctdl_isa ISA() const { return curr_isa; };
    const bool isBadPacket() const;

    // printing
    virtual void toString(std::string &str) const;
    virtual void toStringFmt(const uint32_t fmtFlags, std::string &str) const;
};

inline void EtmV3TrcPacket::UpdateNS(const int NS)
{
    context.curr_NS = NS;
    context.updated = 1;
};

inline void EtmV3TrcPacket::UpdateAltISA(const int AltISA)
{
    context.curr_alt_isa = AltISA;
    context.updated = 1;
}

inline void EtmV3TrcPacket::UpdateHyp(const int Hyp)
{
    context.curr_Hyp = Hyp;
    context.updated = 1;
}

inline void EtmV3TrcPacket::UpdateISA(const rctdl_isa isa)
{
    prev_isa = curr_isa;
    curr_isa = isa;
}

inline void EtmV3TrcPacket::SetType(const rctdl_etmv3_pkt_type p_type)
{
    type = p_type;
}

inline void EtmV3TrcPacket::SetErrType(const rctdl_etmv3_pkt_type e_type)
{
    err_type = type;
    type = e_type;
}

inline const bool EtmV3TrcPacket::isBadPacket() const
{
    return (type >= ETM3_PKT_BAD_SEQUENCE);
}

inline void EtmV3TrcPacket::SetDataOOOTag(const uint8_t tag)
{
    data.ooo_tag = tag;
}

inline void EtmV3TrcPacket::SetDataValue(const uint32_t value)
{
    data.value = value;
}

inline void EtmV3TrcPacket::UpdateContextID(const uint32_t contextID)
{
    context.updated_c = 1;
    context.ctxtID = contextID;
}

inline void EtmV3TrcPacket::UpdateVMID(const uint8_t VMID)
{
    context.updated_v = 1;
    context.VMID = VMID;
}

inline void EtmV3TrcPacket::UpdateDataEndian(const uint8_t BE_Val)
{
    data.be = BE_Val;
    data.update_be = 1;
}

inline void EtmV3TrcPacket::SetCycleCount(const uint32_t cycleCount)
{
    cycle_count = cycleCount;
}

inline void EtmV3TrcPacket::SetISyncReason(const etmv3_isync_reason_t reason)
{
    isync_info.reason = reason;
}

inline void EtmV3TrcPacket::SetISyncHasCC()
{
    isync_info.has_cycle_count = 1;
}

inline void EtmV3TrcPacket::SetISyncIsLSiP()
{
    isync_info.has_LSipAddress = 1;
}



/** @}*/
#endif // ARM_TRC_PKT_ELEM_ETMV3_H_INCLUDED

/* End of File trc_pkt_elem_etmv3.h */

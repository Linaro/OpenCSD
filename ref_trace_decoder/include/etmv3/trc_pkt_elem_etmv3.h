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

    void UpdateAddress(const rctdl_vaddr_t partAddrVal, const int updateBits);
    void SetException(  const rctdl_armv7_exception type, 
                        const uint16_t number, 
                        const bool cancel,
                        const int irq_n = 0,
                        const int resume = 0);
    // packet status interface - get packet info.

    // printing
    virtual void toString(std::string &str) const;
    virtual void toStringFmt(const uint32_t fmtFlags, std::string &str) const;
};
/** @}*/
#endif // ARM_TRC_PKT_ELEM_ETMV3_H_INCLUDED

/* End of File trc_pkt_elem_etmv3.h */

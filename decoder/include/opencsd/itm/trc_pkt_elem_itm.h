/*!
 * \file       trc_pkt_elem_stm.h
 * \brief      OpenCSD : STM packet class.
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

#ifndef ARM_TRC_PKT_ELEM_ITM_H_INCLUDED
#define ARM_TRC_PKT_ELEM_ITM_H_INCLUDED

#include "trc_pkt_types_itm.h"
#include "common/trc_printable_elem.h"
#include "common/trc_pkt_elem_base.h"

  /*!
   * @class ItmTrcPacket
   * @brief ITM trace packet with packet printing functionality
   *
   *  This class allows for the update and access of the current ITM trace
   *  packet
   * 
   *  Based on data structure ocsd_itm_pkt.
   *
   */
class ItmTrcPacket : public TrcPacketBase, public ocsd_itm_pkt, public trcPrintableElem
{
public:
    ItmTrcPacket();
    ~ItmTrcPacket() {};

    ItmTrcPacket& operator =(const ocsd_itm_pkt* p_pkt);
    virtual const void* c_pkt() const { return (const ocsd_itm_pkt*)this; };

    void initPacket();  // init packet to clean state.

    void setPktType(const ocsd_itm_pkt_type pkt_type) { type = pkt_type; };
    void updateErrType(const ocsd_itm_pkt_type err_type);
    void setSrcID(const uint8_t pkt_src_id) { src_id = pkt_src_id; };
    void setValue(const uint32_t pkt_val, const uint8_t val_size_bytes); // size 1-4 bytes
    void setExtValue(const uint64_t pkt_ext_val); // size is always 5 here 

    const ocsd_itm_pkt_type getPktType() const { return type; };
    const uint8_t getSrcID() const { return src_id; };
    const uint8_t getValSize() const { return val_sz; };
    const uint32_t getValue() const { return value;  };
    const uint64_t getExtValue() const;

    // printing
    virtual void toString(std::string& str) const;
    virtual void toStringFmt(const uint32_t fmtFlags, std::string& str) const;

    const bool isBadPacket() const;

private:
    void pktTypeName(const ocsd_itm_pkt_type pkt_type, std::string& name, std::string& desc) const;
    void printValSize(std::string& valStr) const;
    void printDWTPacket(std::string& valStr) const;
    void printTSLocalPacket(std::string& valStr) const;
};



inline const bool ItmTrcPacket::isBadPacket() const
{
    return (bool)(type >= ITM_PKT_BAD_SEQUENCE);
}

inline void ItmTrcPacket::updateErrType(const ocsd_itm_pkt_type err_type)
{
    this->err_type = this->type;    // original type is the err type;
    this->type = err_type;          // mark main type as an error.
}

#endif  // ARM_TRC_PKT_ELEM_ITM_H_INCLUDED

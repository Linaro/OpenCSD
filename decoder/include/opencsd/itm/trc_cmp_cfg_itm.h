/*
 * \file       trc_cmp_cfg_itm.h
 * \brief      OpenCSD : ITM compnent configuration.
 *
 * \copyright  Copyright (c) 2024, ARM Limited. All Rights Reserved.
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

#ifndef ARM_TRC_CMP_CFG_ITM_H_INCLUDED
#define ARM_TRC_CMP_CFG_ITM_H_INCLUDED

#include "trc_pkt_types_itm.h"
#include "common/trc_cs_config.h"

  /** @addtogroup ocsd_protocol_cfg
  @{*/

  /** @name ITM configuration
  @{*/

  /*!
   * @class ITMConfig
   * @brief ITM hardware configuration data.
   *
   *  Represents the programmed and hardware configured state of an ITM device.
   *  Default values set Trace ID at 0.
   */
class ITMConfig : public CSConfig
{
public:
    ITMConfig();    //!< Constructor - creates a default configuration
    ITMConfig(const ocsd_itm_cfg* cfg_regs);
    ~ITMConfig() {};

    // operations to convert to and from C-API structure

    ITMConfig& operator=(const ocsd_itm_cfg* p_cfg);  //!< set from full configuration structure.
    //! cast operator returning struct const reference
    operator const ocsd_itm_cfg& () const { return m_cfg; };
    //! cast operator returning struct const pointer
    operator const ocsd_itm_cfg* () const { return &m_cfg; };

    void setTraceID(const uint8_t traceID);
    virtual const uint8_t getTraceID() const;   //!< Get the CoreSight trace ID. 

    const uint32_t getTSPrescaleValue() const; //!< get the prescaler for the local ts clock

private:
    ocsd_itm_cfg m_cfg;
};

inline ITMConfig::ITMConfig()
{
    m_cfg.reg_tcr = 0;
}

inline ITMConfig::ITMConfig(const ocsd_itm_cfg* cfg_regs)
{
    m_cfg = *cfg_regs;
}

inline ITMConfig& ITMConfig::operator=(const ocsd_itm_cfg* p_cfg)
{
    m_cfg = *p_cfg;
    return *this;
}

inline void ITMConfig::setTraceID(const uint8_t traceID)
{
    uint32_t IDmask = 0x007F0000;
    m_cfg.reg_tcr &= ~IDmask;
    m_cfg.reg_tcr |= (((uint32_t)traceID) << 16) & IDmask;
}

inline const uint8_t ITMConfig::getTraceID() const
{
    return (uint8_t)((m_cfg.reg_tcr >> 16) & 0x7F);
}

inline const uint32_t ITMConfig::getTSPrescaleValue() const
{
    const uint32_t prescaleVals[] = { 1, 4, 16 ,64 };
    int preScaleIdx = 0;

    // prescaler is used with TPIU clock - SWOENA = 1b1 - bit[4]
    if (m_cfg.reg_tcr & 0x10)
        preScaleIdx = ((m_cfg.reg_tcr >> 8) & 0x3);
    return prescaleVals[preScaleIdx];
}

#endif // ARM_TRC_CMP_CFG_ITM_H_INCLUDED


/*
 * \file       trc_cmp_cfg_etmv4.cpp
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

#include "etmv4/trc_cmp_cfg_etmv4.h"

EtmV4Config::EtmV4Config()   
{
    reg_idr0 = 0x28000EA1;
    reg_idr1 = 0x4100F403;   
    reg_idr2 = 0x00000488;       
    reg_idr8 = 0;    
    reg_idr9 = 0;   
    reg_idr10 = 0;   
    reg_idr11 = 0;   
    reg_idr12 = 0;   
    reg_idr13 = 0;   
    reg_configr = 0xC1; 
    reg_traceidr = 0;
    arch_ver = ARCH_V7;
    core_prof = profile_CortexA;

    PrivateInit();
}

EtmV4Config & EtmV4Config::operator=(const rctdl_etmv4_cfg *p_cfg)
{
    this->reg_idr0      = p_cfg->reg_idr0; 
    this->reg_idr1      = p_cfg->reg_idr1;   
    this->reg_idr2      = p_cfg->reg_idr2;     
    this->reg_idr8      = p_cfg->reg_idr8;
    this->reg_idr9      = p_cfg->reg_idr9;   
    this->reg_idr10     = p_cfg->reg_idr10;
    this->reg_idr11     = p_cfg->reg_idr11;
    this->reg_idr12     = p_cfg->reg_idr12;
    this->reg_idr13     = p_cfg->reg_idr13;
    this->reg_configr   = p_cfg->reg_configr;
    this->reg_traceidr  = p_cfg->reg_traceidr;
    this->arch_ver      = p_cfg->arch_ver;   
    this->core_prof     = p_cfg->core_prof;

    PrivateInit();

    return *this;
}

void EtmV4Config::PrivateInit()
{
    m_QSuppCalc = false;
    m_QSuppFilter = false;
    m_QSuppType = Q_NONE;
    m_VMIDSzCalc = false;
    m_VMIDSize = 0;

}

void EtmV4Config::CalcQSupp()
{
    QSuppType qtypes[] = {
        Q_NONE,
        Q_ICOUNT_ONLY,
        Q_NO_ICOUNT_ONLY,
        Q_FULL
    };
    uint8_t Qsupp = (reg_idr0 >> 15) & 0x3;
    m_QSuppType = qtypes[Qsupp];
    m_QSuppFilter = (bool)((reg_idr0 & 0x4000) == 0x4000) && (m_QSuppType != Q_NONE);
    m_QSuppCalc = true;
}

void EtmV4Config::CalcVMIDSize()
{
    uint32_t vmidszF = (reg_idr2 >> 10) & 0x1F;
    if(vmidszF == 1)
        m_VMIDSize = 8;
    else if(MinVersion() > 0)
    {
        if(vmidszF == 2)
            m_VMIDSize = 16;
        else if(vmidszF == 4)
            m_VMIDSize = 32;
    }
    m_VMIDSzCalc = true;
}

/* End of File trc_cmp_cfg_etmv4.cpp */

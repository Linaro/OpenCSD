/*
 * \file       trc_i_decode.h
 * \brief      OpenCSD : 
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
#ifndef ARM_TRC_I_DECODE_H_INCLUDED
#define ARM_TRC_I_DECODE_H_INCLUDED

#include "interfaces/trc_instr_decode_i.h"
#include "interfaces/trc_error_log_i.h"
#include "opencsd/ocsd_if_types.h"

/** Throw error if AA64 opcode top 2 bytes == 0x0000. This range is invalid in AA64 */
#define OCSD_ENV_ERR_ON_AA64_BAD_OPCODE "OPENCSD_ERR_ON_AA64_BAD_OPCODE"


class TrcIDecode : public IInstrDecode
{
public:
    TrcIDecode();
    virtual ~TrcIDecode() {};

    virtual ocsd_err_t DecodeInstruction(ocsd_instr_info* instr_info);

    /* control AA64 checking for invalid opcode */
    void setAA64_errOnBadOpcode(bool bSet);
    void envSetAA64_errOnBadOpcode();

    static void dbgLogMsg(const char* msg);
    static void setErrLogger(ITraceErrorLog* err_log) { p_i_errlog = err_log; };

private:
    ocsd_err_t DecodeA32(ocsd_instr_info *instr_info, struct decode_info *info);
    ocsd_err_t DecodeA64(ocsd_instr_info *instr_info, struct decode_info *info);
    ocsd_err_t DecodeT32(ocsd_instr_info *instr_info, struct decode_info *info);

    bool aa64_err_bad_opcode;   //!< error if aa64 opcode is in invalid range (top 2 bytes = 0x0000).

    static ITraceErrorLog* p_i_errlog;
};

inline void TrcIDecode::setAA64_errOnBadOpcode(bool bSet)
{
    aa64_err_bad_opcode = bSet;
}

#endif // ARM_TRC_I_DECODE_H_INCLUDED

/* End of File trc_i_decode.h */

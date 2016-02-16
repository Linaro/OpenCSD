/*
 * \file       trc_gen_elem.cpp
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

#include "trc_gen_elem.h"

#include <string>
#include <sstream>
#include <iomanip>

static const char *s_elem_descs[][2] =  
{
    {"RCTDL_GEN_TRC_ELEM_UNKNOWN","Unknown trace element - default value or indicate error in stream to client."},
    {"RCTDL_GEN_TRC_ELEM_NO_SYNC","Waiting for sync - either at start of decode, or after overflow / bad packet"},
    {"RCTDL_GEN_TRC_ELEM_TRACE_ON","Start of trace - beginning of elements or restart after discontinuity (overflow, trace filtering)."},
    {"RCTDL_GEN_TRC_ELEM_EO_TRACE","End of the available trace in the buffer."},
    {"RCTDL_GEN_TRC_ELEM_PE_CONTEXT","PE status update / change (arch, ctxtid, vmid etc)."},
    {"RCTDL_GEN_TRC_ELEM_INSTR_RANGE","Traced N consecutive instructions from addr (no intervening events or data elements), may have data assoc key"},
    {"RCTDL_GEN_TRC_ELEM_ADDR_NACC","Tracing in inaccessible memory area."},
    {"RCTDL_GEN_TRC_ELEM_EXCEPTION","Exception"},
    {"RCTDL_GEN_TRC_ELEM_EXCEPTION_RET","Expection return"},
    {"RCTDL_GEN_TRC_ELEM_TIMESTAMP","Timestamp - preceding elements happeded before this time."},
    {"RCTDL_GEN_TRC_ELEM_CYCLE_COUNT","Cycle count - cycles since last cycle count value - associated with a preceding instruction range."},
    {"RCTDL_GEN_TRC_ELEM_EVENT","Event - numbered event or trigger"}
};

static const char *instr_type[] = {
    "--- ",
    "BR  ",
    "iBR ",
    "ISB ",
    "DSB.DMB"
};

static const char *s_trace_on_reason[] = {
    "begin or filter",
    "overflow",
    "debug restart"
};

void RctdlTraceElement::toString(std::string &str) const
{
    std::ostringstream oss;
    int num_str = ((sizeof(s_elem_descs) / sizeof(const char *)) / 2);
    int typeIdx = (int)this->elem_type;
    if(typeIdx < num_str)
    {
        oss << s_elem_descs[typeIdx][0] << "(";
        switch(elem_type)
        {
        case RCTDL_GEN_TRC_ELEM_INSTR_RANGE:
            oss << "exec range=0x" << std::hex << st_addr << ":[0x" << en_addr << "] ";
            oss << ((last_instr_exec == 1) ? "E " : "N ");
            oss << instr_type[last_i_type];
            break;

        case RCTDL_GEN_TRC_ELEM_ADDR_NACC:
            oss << " 0x" << std::hex << st_addr << " ";
            break;

        case RCTDL_GEN_TRC_ELEM_EXCEPTION:
            if(en_addr != st_addr)
            {
                oss << "exec range=0x" << std::hex << st_addr << ":[0x" << en_addr << "]; ";
            }
            oss << "pref ret addr:0x" << std::hex << en_addr << "; excep num (0x" << std::setfill('0') << std::setw(2) << gen_value;
            break;

        case RCTDL_GEN_TRC_ELEM_PE_CONTEXT:
            oss << "EL" << std::dec << (int)(context.exception_level) << (context.security_level == rctdl_sec_secure ? " S; " : "N; ") << (context.bits64 ? "AArch64; " : "AArch32; ");
            if(context.vmid_valid)
                oss << "VMID=0x" << std::hex << context.vmid << "; ";
            if(context.ctxt_id_valid)
                oss << "CTXTID=0x" << std::hex << context.context_id << "; ";
            break;

        case  RCTDL_GEN_TRC_ELEM_TRACE_ON:
            oss << " [" << s_trace_on_reason[trace_on_reason] << "]";
            break;

        default: break;
        }
        oss << ")";
    }
    else
    {
        oss << "RCTDL_GEN_TRC_ELEM??: index out of range.";
    }
    str = oss.str();
}

RctdlTraceElement &RctdlTraceElement::operator =(const rctdl_generic_trace_elem* p_elem)
{
    *dynamic_cast<rctdl_generic_trace_elem*>(this) = *p_elem;
    return *this;
}

/*
void RctdlTraceElement::toString(const rctdl_generic_trace_elem *p_elem, std::string &str)
{
    RctdlTraceElement elem;
    elem = p_elem;
    elem.toString(str);
}
*/
/* End of File trc_gen_elem.cpp */

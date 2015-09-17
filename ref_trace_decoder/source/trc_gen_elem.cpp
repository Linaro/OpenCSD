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

static const char *s_elem_descs[][2] =  
{
    {"RCTDL_GEN_TRC_ELEM_UNKNOWN","Unknown trace element - default value or indicate error in stream to client."},
    {"RCTDL_GEN_TRC_ELEM_NO_SYNC","Waiting for sync - either at start of decode, or after overflow / bad packet"},
    {"RCTDL_GEN_TRC_ELEM_TRACE_ON","Start of trace - beginning of elements or restart after discontinuity (overflow, trace filtering)."},
    {"RCTDL_GEN_TRC_ELEM_TRACE_OVERFLOW","Trace overflow - indicates discontinuity due to too much trace - normally followed by trace on."},
    {"RCTDL_GEN_TRC_ELEM_EO_TRACE","End of the available trace in the buffer."},
    {"RCTDL_GEN_TRC_ELEM_PE_CONTEXT","PE status update / change (arch, ctxtid, vmid etc)."},
    {"RCTDL_GEN_TRC_ELEM_INSTR_RANGE","Traced N consecutive instructions from addr (no intervening events or data elements), may have data assoc key"},
    {"RCTDL_GEN_TRC_ELEM_ADDR_NACC","Tracing in inaccessible memory area."},
    {"RCTDL_GEN_TRC_ELEM_EXCEPTION","Exception"},
    {"RCTDL_GEN_TRC_ELEM_EXCEPTION_RET","Expection return"},
    {"RCTDL_GEN_TRC_ELEM_TIMESTAMP","Timestamp - preceding elements happeded before this time."},
    {"RCTDL_GEN_TRC_ELEM_CYCLE_COUNT","Cycle count - cycles since last cycle count value - associated with a preceding instruction range."},
    {"RCTDL_GEN_TRC_ELEM_TS_WITH_CC","Timestamp with Cycle count - preceding elements happened before timestamp, cycle count associated with the timestamp, cycle count is associated with TS and since last cycle count value"},
    {"RCTDL_GEN_TRC_ELEM_EVENT","Event - numbered event or trigger"}
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
            oss << "exec range=0x" << std::hex << st_addr << ":0x" << en_addr-1 << " ";
            break;

        case RCTDL_GEN_TRC_ELEM_ADDR_NACC:
            oss << " 0x" << std::hex << st_addr << " ";
            break;

        case RCTDL_GEN_TRC_ELEM_EXCEPTION:
            if(en_addr != st_addr)
            {
                oss << "exec range=0x" << std::hex << st_addr << ":0x" << en_addr-1 << "; ";
            }
            oss << "pref ret addr:0x" << std::hex << en_addr << "; excep num " << gen_value;
            break;

        default: break;
        }
        oss << ")" << std::endl;
    }
    else
    {
        oss << "RCTDL_GEN_TRC_ELEM??: index out of range." << std::endl;
    }
    str = oss.str();
}

/* End of File trc_gen_elem.cpp */

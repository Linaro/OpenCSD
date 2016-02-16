/*!
 * \file       trc_gen_elem.h
 * \brief      Reference CoreSight Trace Decoder : Decoder Generic trace element output class.
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
#ifndef ARM_TRC_GEN_ELEM_H_INCLUDED
#define ARM_TRC_GEN_ELEM_H_INCLUDED

#include "trc_gen_elem_types.h"
#include "trc_printable_elem.h"
/** @addtogroup gen_trc_elem 
@{*/

/*!
 * @class RctdlTraceElement
 * @brief Generic trace element class
 * 
 */
class RctdlTraceElement : public trcPrintableElem, public rctdl_generic_trace_elem
{
public:
    RctdlTraceElement();
    RctdlTraceElement(rctdl_gen_trc_elem_t type);
    virtual ~RctdlTraceElement() {};

    void init();

    const rctdl_gen_trc_elem_t getType() const { return elem_type; };
    void setType(const rctdl_gen_trc_elem_t type);
    void setContext(const rctdl_pe_context &new_context) { context = new_context; };
    void setCycleCount(const uint32_t cycleCount);
    void setEvent(const event_t ev_type, const uint16_t number);

    virtual void toString(std::string &str) const;

    RctdlTraceElement &operator =(const rctdl_generic_trace_elem* p_elem);

};

inline RctdlTraceElement::RctdlTraceElement(rctdl_gen_trc_elem_t type)    
{
    elem_type = type;
}

inline RctdlTraceElement::RctdlTraceElement()    
{
    elem_type = RCTDL_GEN_TRC_ELEM_UNKNOWN;
}

inline void RctdlTraceElement::setCycleCount(const uint32_t cycleCount)
{
    cycle_count = cycleCount;
    has_cc = 1;
}

inline void RctdlTraceElement::setEvent(const event_t ev_type, const uint16_t number)
{
    trace_event.ev_type = (uint16_t)ev_type;
    trace_event.ev_number = ev_type == EVENT_NUMBERED ? number : 0;
}

inline void RctdlTraceElement::setType(const rctdl_gen_trc_elem_t type) 
{ 
    // set the type and clear down the per element flags
    elem_type = type;
    has_cc = 0;
    last_instr_exec = 0;
    last_i_type = RCTDL_INSTR_OTHER;
    i_type_with_link = 0;
}

inline void RctdlTraceElement::init()
{
    context.ctxt_id_valid = 0;
    context.vmid_valid = 0;
    context.el_valid = 0;

    cpu_freq_change = 0;
    has_cc = 0;
    last_instr_exec = 0;
    i_type_with_link = 0;
}

/** @}*/

#endif // ARM_TRC_GEN_ELEM_H_INCLUDED

/* End of File trc_gen_elem.h */

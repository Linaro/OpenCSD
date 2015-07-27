/*
 * \file       trc_gen_elem.h
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
#ifndef ARM_TRC_GEN_ELEM_H_INCLUDED
#define ARM_TRC_GEN_ELEM_H_INCLUDED

#include "trc_gen_elem_types.h"
#include "trc_printable_elem.h"
/** @addtogroup gen_trc_elem 
@{*/

/*!
 * @class RctdlTraceElement
 * @brief Base class for all trace elements output.
 * 
 */
class RctdlTraceElement : public trcPrintableElem, public rctdl_generic_trace_elem
{
public:
    RctdlTraceElement(rctdl_gen_trc_elem_t type);
    virtual ~RctdlTraceElement();

    const rctdl_gen_trc_elem_t getType() const { return elem_type; };

};

// End of trace, not much more to be said - perhaps add in information on incomplete packets here?.
class RctdlTE_EOT : public RctdlTraceElement
{
public:
    RctdlTE_EOT() : RctdlTraceElement(RCTDL_GEN_TRC_ELEM_EO_TRACE) {};
    ~RctdlTE_EOT() {};    

    virtual void toString(std::string &outStr) const;
};

inline void RctdlTE_EOT::toString(std::string &outStr) const
{
    outStr = "End Of Trace";
}

/** @}*/

#endif // ARM_TRC_GEN_ELEM_H_INCLUDED

/* End of File trc_gen_elem.h */

/*
 * \file       gen_elem_printer.h
 * \brief      OpenCSD : Generic element printer class.
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
#ifndef ARM_GEN_ELEM_PRINTER_H_INCLUDED
#define ARM_GEN_ELEM_PRINTER_H_INCLUDED

#include "opencsd.h"

class TrcGenericElementPrinter : public ItemPrinter, public ITrcGenElemIn
{
public:
    TrcGenericElementPrinter();
    virtual ~TrcGenericElementPrinter() {};

    virtual ocsd_datapath_resp_t TraceElemIn(const ocsd_trc_index_t index_sop,
                                              const uint8_t trc_chan_id,         
                                              const OcsdTraceElement &elem);

    // funtionality to test wait / flush mechanism
    void ackWait() { m_needWaitAck = false; };
    const bool needAckWait() const { return m_needWaitAck; };
    void set_collect_stats() { m_collect_stats = true; };
    void printStats();

protected:
    bool m_needWaitAck;
    bool m_collect_stats;  // collect stats on packets processed
    int m_packet_counts[(int)OCSD_GEN_TRC_ELEM_CUSTOM + 1];
};

#endif // ARM_GEN_ELEM_PRINTER_H_INCLUDED

/* End of File gen_elem_printer.h */

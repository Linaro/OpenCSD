/*
 * \file       gen_elem_printer.cpp
 * \brief      OpenCSD : Generic element printer class.
 *
 * \copyright  Copyright (c) 2015,2023 ARM Limited. All Rights Reserved.
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

#include <string>
#include <sstream>
#include <iomanip>

#include "opencsd.h"

TrcGenericElementPrinter::TrcGenericElementPrinter() :
    m_needWaitAck(false),
    m_collect_stats(false)
{
    for (int i = 0; i <= (int)OCSD_GEN_TRC_ELEM_CUSTOM; i++)
        m_packet_counts[i] = 0;
}

ocsd_datapath_resp_t TrcGenericElementPrinter::TraceElemIn(const ocsd_trc_index_t index_sop,
    const uint8_t trc_chan_id,
    const OcsdTraceElement& elem)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    if (m_collect_stats)
        m_packet_counts[(int)elem.getType()]++;

    if (is_muted())
        return resp;

    std::string elemStr;
    std::ostringstream oss;
    oss << "Idx:" << index_sop << "; ID:" << std::hex << (uint32_t)trc_chan_id << "; ";
    elem.toString(elemStr);
    oss << elemStr << std::endl;
    itemPrintLine(oss.str());

    // funtionality to test wait / flush mechanism
    if (m_needWaitAck)
    {
        oss.str("");
        oss << "WARNING: Generic Element Printer; New element without previous _WAIT acknowledged\n";
        itemPrintLine(oss.str());
        m_needWaitAck = false;
    }

    if (getTestWaits())
    {
        resp = OCSD_RESP_WAIT; // return _WAIT for the 1st N packets.
        decTestWaits();
        m_needWaitAck = true;
    }
    return resp;
}

void TrcGenericElementPrinter::printStats()
{
    static const char* gen_elem_packet_names[] = {
    "OCSD_GEN_TRC_ELEM_UNKNOWN",
    "OCSD_GEN_TRC_ELEM_NO_SYNC",
    "OCSD_GEN_TRC_ELEM_TRACE_ON",
    "OCSD_GEN_TRC_ELEM_EO_TRACE",
    "OCSD_GEN_TRC_ELEM_PE_CONTEXT",
    "OCSD_GEN_TRC_ELEM_INSTR_RANGE",
    "OCSD_GEN_TRC_ELEM_I_RANGE_NOPATH",
    "OCSD_GEN_TRC_ELEM_ADDR_NACC",
    "OCSD_GEN_TRC_ELEM_ADDR_UNKNOWN",
    "OCSD_GEN_TRC_ELEM_EXCEPTION",
    "OCSD_GEN_TRC_ELEM_EXCEPTION_RET",
    "OCSD_GEN_TRC_ELEM_TIMESTAMP",
    "OCSD_GEN_TRC_ELEM_CYCLE_COUNT",
    "OCSD_GEN_TRC_ELEM_EVENT",
    "OCSD_GEN_TRC_ELEM_SWTRACE",
    "OCSD_GEN_TRC_ELEM_SYNC_MARKER",
    "OCSD_GEN_TRC_ELEM_MEMTRANS",
    "OCSD_GEN_TRC_ELEM_INSTRUMENTATION",
    "OCSD_GEN_TRC_ELEM_CUSTOM",
    };

    std::ostringstream oss;

    oss << "Generic Packets processed:-\n";
    for (int i = 0; i <= OCSD_GEN_TRC_ELEM_CUSTOM; i++)
    {
        oss << gen_elem_packet_names[i] << " : " << m_packet_counts[i] << "\n";
    }
    oss << "\n\n";

    itemPrintLine(oss.str());
}



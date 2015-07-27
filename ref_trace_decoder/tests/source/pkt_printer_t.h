/*
 * \file       pkt_printer_t.h
 * \brief      Reference CoreSight Trace Decoder : Test packet printer.
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

#ifndef ARM_PKT_PRINTER_T_H_INCLUDED
#define ARM_PKT_PRINTER_T_H_INCLUDED

#include "rctdl.h"
#include "item_printer.h"

#include <string>
#include <sstream>

template<class P>
class PacketPrinter : public IPktDataIn<P>, public ItemPrinter
{
public:
    PacketPrinter(const uint8_t trcID);
    PacketPrinter(const uint8_t trcID, rctdlMsgLogger *pMsgLogger);
    virtual ~PacketPrinter();


    virtual rctdl_datapath_resp_t PacketDataIn( const rctdl_datapath_op_t op,
                                        const rctdl_trc_index_t index_sop,                                        
                                        const P *p_packet_in);

private:
    uint8_t m_trcID;
};

template<class P> PacketPrinter<P>::PacketPrinter(uint8_t trcID) : 
    m_trcID(trcID)
{
}

template<class P> PacketPrinter<P>::PacketPrinter(const uint8_t trcID, rctdlMsgLogger *pMsgLogger) :
    m_trcID(trcID)
{
    setMessageLogger(pMsgLogger);
}

template<class P> PacketPrinter<P>::~PacketPrinter()
{
}

template<class P> rctdl_datapath_resp_t PacketPrinter<P>::PacketDataIn( const rctdl_datapath_op_t op,
                                        const rctdl_trc_index_t index_sop,
                                        const P *p_packet_in)
{
    std::string pktstr;
    std::ostringstream oss;

    switch(op)
    {
    case RCTDL_OP_DATA:
        p_packet_in->toString(pktstr);
        oss << "Idx:" << index_sop << "; ID:"<< m_trcID << "\t" << pktstr << std::endl;
        itemPrintLine(oss.str());
        break;

    case RCTDL_OP_EOT:
        oss <<"ID:"<< m_trcID << "\tEND OF TRACE DATA\n";
        break;

    case RCTDL_OP_FLUSH:
        oss <<"ID:"<< m_trcID << "\tFLUSH operation on trace decode path\n";
        break;

    case RCTDL_OP_RESET:
        oss <<"ID:"<< m_trcID << "\tRESET operation on trace decode path\n";
        break;
    }

    itemPrintLine(oss.str());
    return RCTDL_RESP_CONT; // always return continue.
}

#endif // ARM_PKT_PRINTER_T_H_INCLUDED

/* End of File pkt_printer_t.h */

/*
* \file       trc_print_fact.cpp
* \brief      OpenCSD : Trace Packet printer factory.
*
* \copyright  Copyright (c) 2017, ARM Limited. All Rights Reserved.
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

#include "pkt_printers/trc_print_fact.h"

std::vector<ItemPrinter *> PktPrinterFact::sm_printers;

RawFramePrinter * PktPrinterFact::createRawFramePrinter(ocsdMsgLogger *pMsgLogger /*= 0*/)
{
    RawFramePrinter *pPrinter = 0;
    pPrinter = new (std::nothrow)RawFramePrinter();
    SavePrinter(pPrinter, pMsgLogger);
    return pPrinter;
}

TrcGenericElementPrinter *PktPrinterFact::createGenElemPrinter(ocsdMsgLogger *pMsgLogger /*= 0*/)
{
    TrcGenericElementPrinter *pPrinter = 0;
    pPrinter = new (std::nothrow)TrcGenericElementPrinter();
    SavePrinter(pPrinter, pMsgLogger);
    return pPrinter;
}

ItemPrinter *PktPrinterFact::createProtocolPrinter(ocsd_trace_protocol_t protocol, uint8_t CSID, ocsdMsgLogger *pMsgLogger /*= 0*/)
{
    ItemPrinter *pPrinter = 0;
    switch (protocol)
    {
    case OCSD_PROTOCOL_ETMV4I:
        pPrinter = new (std::nothrow) PacketPrinter<EtmV4ITrcPacket>(CSID);
        break;
    case OCSD_PROTOCOL_ETMV3:
        pPrinter = new (std::nothrow) PacketPrinter<EtmV3TrcPacket>(CSID);
        break;
    case OCSD_PROTOCOL_PTM:
        pPrinter = new (std::nothrow) PacketPrinter<PtmTrcPacket>(CSID);
        break;
    case OCSD_PROTOCOL_STM:
        pPrinter = new (std::nothrow) PacketPrinter<StmTrcPacket>(CSID);
        break;
    default:
        break;
    }
    SavePrinter(pPrinter, pMsgLogger);
    return pPrinter;
}

const int PktPrinterFact::numPrinters()
{
    return sm_printers.size();
}

std::vector<ItemPrinter *> &PktPrinterFact::getPrinterList()
{
    return sm_printers;
}

void PktPrinterFact::SavePrinter(ItemPrinter *pPrinter, ocsdMsgLogger *pMsgLogger)
{
    if (pPrinter)
    {
        pPrinter->setMessageLogger(pMsgLogger);
        sm_printers.push_back((ItemPrinter *)pPrinter);
    }
}

void PktPrinterFact::destroyAllPrinters()
{
    std::vector<ItemPrinter *>::iterator it;
    it = sm_printers.begin();
    while (it != sm_printers.end())
    {
        delete *it;
        it++;
    }
    sm_printers.clear();
}

/* end of file  trc_print_fact.cpp */

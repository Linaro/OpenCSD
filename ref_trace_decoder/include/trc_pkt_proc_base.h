/*
 * \file       trc_pkt_proc_base.h
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

#ifndef ARM_TRC_PKT_PROC_BASE_H_INCLUDED
#define ARM_TRC_PKT_PROC_BASE_H_INCLUDED

#include "interfaces/trc_data_raw_in_i.h"
#include "interfaces/trc_pkt_in_i.h"
#include "interfaces/trc_pkt_raw_in_i.h"
#include "interfaces/trc_indexer_pkt_i.h"

#include "trc_component.h"
#include "comp_attach_pt_t.h"

/** @defgroup rctdl_pkt_proc  Reference CoreSight Trace Decoder Library : Packet Processors.

    @brief Classes providing Protocol Packet Processing capability.

    Packet processors take an incoming byte stream and convert into discrete packets for the
    required trace protocol.
@{*/

class TrcPktProcI : public TraceComponent, public ITrcDataIn
{
public:
    TrcPktProcI(const char *component_name);
    TrcPktProcI(const char *component_name, int instIDNum);
    virtual ~TrcPktProcI() {}; 

    /* generic trace data input interface */
    virtual rctdl_datapath_resp_t TraceDataIn(  const rctdl_datapath_op_t op,
                                                const rctdl_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed) = 0;

protected:

    /* implementation packet processing interface */
    virtual rctdl_datapath_resp_t processData(  const rctdl_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed) = 0;
    virtual rctdl_datapath_resp_t onEOT() = 0;
    virtual rctdl_datapath_resp_t onReset() = 0;
    virtual rctdl_datapath_resp_t onFlush() = 0;
    virtual rctdl_err_t onProtocolConfig() = 0;
};

inline TrcPktProcI::TrcPktProcI(const char *component_name) :
    TraceComponent(component_name)
{
}

inline TrcPktProcI::TrcPktProcI(const char *component_name, int instIDNum) :
    TraceComponent(component_name,instIDNum)
{
}

/*!
 * @class TrcPktProcBase
 * @brief Packet Processor base class. Provides common infrastructure and interconnections for packet processors.
 * 
 *  The class is a templated base class. 
 *  P  - this is the packet object class.
 *  Pt - this is the packet type class.
 *  Pc - this is the packet configuration class.
 * 
 *  implementations will provide concrete classes for each of these to operate under the common infrastructures.
 *  The base provides the trace data in (ITrcDataIn) interface and operates on the incoming operation type.
 *  
 *  Implementions override the 'onFn()' and data process functions, with the base class ensuring consistent
 *  ordering of operations.
 * 
 */
template <class P, class Pt, class Pc> 
class TrcPktProcBase : public TrcPktProcI
{
public:
    TrcPktProcBase(const char *component_name);
    TrcPktProcBase(const char *component_name, int instIDNum);
    virtual ~TrcPktProcBase() {}; 

        /* generic trace data input interface */
    virtual rctdl_datapath_resp_t TraceDataIn(  const rctdl_datapath_op_t op,
                                                const rctdl_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed);


    /* component attachment points */ 
    componentAttachPt<IPktDataIn<P>> *getPacketOutAttachPt()  { return &m_pkt_out_i; };
    componentAttachPt<ITrcPktIndexer<Pt>> *getTraceIDIndexerAttachPt() { return &m_pkt_indexer_i; };
    componentAttachPt<IPktRawDataMon<Pt>> *getRawPacketMonAttachPt() { return &m_pkt_raw_mon_i; };
   
    /* protocol configuration */
    virtual rctdl_err_t setProtocolConfig(const Pc *config);
    virtual const Pc *getProtocolConfig() const { return m_config; };

protected:

    /* data output functions */
    rctdl_datapath_resp_t outputDecodedPacket(const rctdl_trc_index_t index_sop, const P *pkt);
    
    void outputRawPacketToMonitor( const rctdl_trc_index_t index_sop,
                                   const P *pkt,
                                   const uint32_t size,
                                   const uint8_t *p_data);

    void indexPacket(const rctdl_trc_index_t index_sop, const Pt *packet_type);

    rctdl_datapath_resp_t outputOnAllInterfaces(const rctdl_trc_index_t index_sop, const P *pkt, const Pt *pkt_type, std::vector<uint8_t> pktdata);

    /* the protocol configuration */
    const Pc *m_config;

private:
    /* decode control */
    rctdl_datapath_resp_t Reset();
    rctdl_datapath_resp_t Flush();
    rctdl_datapath_resp_t EOT();

    componentAttachPt<IPktDataIn<P>> m_pkt_out_i;    
    componentAttachPt<IPktRawDataMon<P>> m_pkt_raw_mon_i;

    componentAttachPt<ITrcPktIndexer<Pt>> m_pkt_indexer_i;
};

template<class P,class Pt, class Pc> TrcPktProcBase<P, Pt, Pc>::TrcPktProcBase(const char *component_name) : 
    TrcPktProcI(component_name),
    m_config(0)
{
}

template<class P,class Pt, class Pc> TrcPktProcBase<P, Pt, Pc>::TrcPktProcBase(const char *component_name, int instIDNum) : 
    TrcPktProcI(component_name, instIDNum),
    m_config(0)
{
}

template<class P,class Pt, class Pc> rctdl_datapath_resp_t TrcPktProcBase<P, Pt, Pc>::TraceDataIn(  const rctdl_datapath_op_t op,
                                                const rctdl_trc_index_t index,
                                                const uint32_t dataBlockSize,
                                                const uint8_t *pDataBlock,
                                                uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    switch(op)
    {
    case RCTDL_OP_DATA:
        if((dataBlockSize == 0) || (pDataBlock == 0) || (numBytesProcessed == 0))
        {
            LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_INVALID_PARAM_VAL));
            resp = RCTDL_RESP_FATAL_INVALID_PARAM;
        }
        else
            resp = processData(index,dataBlockSize,pDataBlock,numBytesProcessed);
        break;

    case RCTDL_OP_EOT:
        resp = EOT();
        break;

    case RCTDL_OP_FLUSH:
        resp = Flush();
        break;

    case RCTDL_OP_RESET:
        resp = Reset();
        break;

    default:
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_INVALID_PARAM_VAL));
        resp = RCTDL_RESP_FATAL_INVALID_OP;
        break;
    }
    return resp;
}


template<class P,class Pt, class Pc> rctdl_datapath_resp_t TrcPktProcBase<P, Pt, Pc>::Reset()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;

    // reset the trace decoder attachment on main data path.
    if(m_pkt_out_i.hasAttachedAndEnabled())
        resp = m_pkt_out_i.first()->PacketDataIn(RCTDL_OP_RESET,0,0);

    // reset the packet processor implmentation
    if(!RCTDL_DATA_RESP_IS_FATAL(resp))
        resp = onReset();

    // packet monitor
    if(m_pkt_raw_mon_i.hasAttachedAndEnabled())
        m_pkt_raw_mon_i.first()->RawPacketDataMon(RCTDL_OP_RESET,0,0,0,0);

    return resp;
}

template<class P,class Pt, class Pc> rctdl_datapath_resp_t TrcPktProcBase<P, Pt, Pc>::Flush()
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    rctdl_datapath_resp_t resplocal = RCTDL_RESP_CONT;

    // the trace decoder attachment on main data path.
    if(m_pkt_out_i.hasAttachedAndEnabled())
        resp = m_pkt_out_i.first()->PacketDataIn(RCTDL_OP_FLUSH,0,0); // flush up the data path first.

    // if the connected components are flushed, not flush this one.
    if(RCTDL_DATA_RESP_IS_CONT(resp))
        resplocal = onFlush();      // local flush

    return (resplocal > resp) ?  resplocal : resp;
}

template<class P,class Pt, class Pc> rctdl_datapath_resp_t TrcPktProcBase<P, Pt, Pc>::EOT()
{
    rctdl_datapath_resp_t resp = onEOT();   // local EOT - mark any part packet as incomplete type and prepare to send

    // the trace decoder attachment on main data path.
    if(m_pkt_out_i.hasAttachedAndEnabled() && !RCTDL_DATA_RESP_IS_FATAL(resp))
        resp = m_pkt_out_i.first()->PacketDataIn(RCTDL_OP_EOT,0,0);

    // packet monitor
    if(m_pkt_raw_mon_i.hasAttachedAndEnabled())
        m_pkt_raw_mon_i.first()->RawPacketDataMon(RCTDL_OP_EOT,0,0,0,0);

    return resp;
}

template<class P,class Pt, class Pc> rctdl_datapath_resp_t TrcPktProcBase<P, Pt, Pc>::outputDecodedPacket(const rctdl_trc_index_t index, const P *pkt)
{
    // send a complete packet over the primary data path
     rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
     if(m_pkt_out_i.hasAttachedAndEnabled())
         resp = m_pkt_out_i.first()->PacketDataIn(RCTDL_OP_DATA,index,pkt);
     return resp;
}

template<class P,class Pt, class Pc> void TrcPktProcBase<P, Pt, Pc>::outputRawPacketToMonitor( 
                                    const rctdl_trc_index_t index_sop,
                                    const P *pkt,
                                    const uint32_t size,
                                    const uint8_t *p_data)
{
    // packet monitor - this cannot return CONT / WAIT, but does get the raw packet data.
    if(m_pkt_raw_mon_i.hasAttachedAndEnabled())
        m_pkt_raw_mon_i.first()->RawPacketDataMon(RCTDL_OP_DATA,index_sop,pkt,size,p_data);
}

template<class P,class Pt, class Pc> void TrcPktProcBase<P, Pt, Pc>::indexPacket(const rctdl_trc_index_t index_sop, const Pt *packet_type)
{
    // packet indexer - cannot return CONT / WAIT, just gets the current index and type.
    if(m_pkt_indexer_i.hasAttachedAndEnabled())
        m_pkt_indexer_i.first()->TracePktIndex(index_sop,packet_type);
}

template<class P,class Pt, class Pc> rctdl_datapath_resp_t TrcPktProcBase<P, Pt, Pc>::outputOnAllInterfaces(const rctdl_trc_index_t index_sop, const P *pkt, const Pt *pkt_type, std::vector<uint8_t> pktdata)
{
    indexPacket(index_sop,pkt_type);
    outputRawPacketToMonitor(index_sop,pkt,(uint32_t)pktdata.size(),&pktdata[0]);
    return outputDecodedPacket(index_sop,pkt);
}

template<class P,class Pt, class Pc> rctdl_err_t TrcPktProcBase<P, Pt, Pc>::setProtocolConfig(const Pc *config)
{
    rctdl_err_t err = RCTDL_ERR_INVALID_PARAM_VAL;
    if(config != 0)
    {
        m_config = config;
        err = onProtocolConfig();
    }
    return err;
}

/** @}*/

#endif // ARM_TRC_PKT_PROC_BASE_H_INCLUDED

/* End of File trc_pkt_proc_base.h */

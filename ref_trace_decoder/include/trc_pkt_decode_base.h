/*!
 * \file       trc_pkt_decode_base.h
 * \brief      Reference CoreSight Trace Decoder : Trace Packet decoder base class.
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

#ifndef ARM_TRC_PKT_DECODE_BASE_H_INCLUDED
#define ARM_TRC_PKT_DECODE_BASE_H_INCLUDED

#include "trc_component.h"
#include "comp_attach_pt_t.h"

#include "interfaces/trc_pkt_in_i.h"
#include "interfaces/trc_gen_elem_in_i.h"
#include "interfaces/trc_tgt_mem_access_i.h"
#include "interfaces/trc_instr_decode_i.h"


/** @defgroup rctdl_pkt_decode Reference CoreSight Trace Decoder Library : Packet Decoders.

    @brief Classes providing Protocol Packet Decoding capability.

    Packet decoders convert incoming protocol packets from a packet processor,
    into generic trace elements to be output to an analysis program.

    Packet decoders can be:-
    - PE decoders - converting ETM or PTM packets into instruction and data trace elements
    - SW stimulus decoder - converting STM or ITM packets into software generated trace elements.
    - Bus decoders - converting HTM packets into bus transaction elements.

@{*/


class TrcPktDecodeI : public TraceComponent
{
public:
    TrcPktDecodeI(const char *component_name);
    TrcPktDecodeI(const char *component_name, int instIDNum);

    componentAttachPt<ITrcGenElemIn> *getTraceElemOutAttachPt() { return &m_trace_elem_out; };
    componentAttachPt<ITargetMemAccess> *getMemoryAccessAttachPt() { return &m_mem_access; };
    componentAttachPt<IInstrDecode> *getInstrDecodeAttachPt() { return &m_instr_decode; };


protected:

    /* implementation packet decoding interface */
    virtual rctdl_datapath_resp_t processPacket() = 0;
    virtual rctdl_datapath_resp_t onEOT() = 0;
    virtual rctdl_datapath_resp_t onReset() = 0;
    virtual rctdl_datapath_resp_t onFlush() = 0;
    virtual rctdl_err_t onProtocolConfig() = 0;
    virtual const uint8_t getCoreSightTraceID() = 0;

    const bool checkInit();

    /* data output */
    rctdl_datapath_resp_t outputTraceElement(const RctdlTraceElement &elem);    // use current index
    rctdl_datapath_resp_t outputTraceElementIdx(rctdl_trc_index_t idx, const RctdlTraceElement &elem); // use supplied index (where decoder caches elements) 

    /* target access */
    rctdl_err_t accessMemory(const rctdl_vaddr_t address, const rctdl_mem_space_acc_t mem_space, uint32_t *num_bytes, uint8_t *p_buffer);

    /* instruction decode */
    rctdl_err_t instrDecode(rctdl_instr_info *instr_info);

    componentAttachPt<ITrcGenElemIn> m_trace_elem_out;
    componentAttachPt<ITargetMemAccess> m_mem_access;
    componentAttachPt<IInstrDecode> m_instr_decode;

    rctdl_trc_index_t   m_index_curr_pkt;

    bool m_decode_init_ok;  //!< set true if all attachments in place for decode. (remove checks in main throughput paths)
    bool m_config_init_ok;  //!< set true if config set.

};

inline TrcPktDecodeI::TrcPktDecodeI(const char *component_name) : 
    TraceComponent(component_name),
    m_index_curr_pkt(0),
    m_decode_init_ok(false),
    m_config_init_ok(false)
{
}

inline TrcPktDecodeI::TrcPktDecodeI(const char *component_name, int instIDNum) :
    TraceComponent(component_name, instIDNum),
    m_index_curr_pkt(0),
    m_decode_init_ok(false),
    m_config_init_ok(false)
{
}

inline const bool TrcPktDecodeI::checkInit()
{
    if(!m_decode_init_ok)
    {
        m_decode_init_ok =  m_config_init_ok &&
                            m_trace_elem_out.hasAttachedAndEnabled() &&
                            m_mem_access.hasAttachedAndEnabled() &&
                            m_instr_decode.hasAttachedAndEnabled();
    }
    return m_decode_init_ok;
}

inline rctdl_datapath_resp_t TrcPktDecodeI::outputTraceElement(const RctdlTraceElement &elem)
{
    return m_trace_elem_out.first()->TraceElemIn(m_index_curr_pkt,getCoreSightTraceID(), elem);
}

inline rctdl_datapath_resp_t TrcPktDecodeI::outputTraceElementIdx(rctdl_trc_index_t idx, const RctdlTraceElement &elem)
{
    return m_trace_elem_out.first()->TraceElemIn(idx, getCoreSightTraceID(), elem);
}

inline rctdl_err_t TrcPktDecodeI::instrDecode(rctdl_instr_info *instr_info)
{
    return m_instr_decode.first()->DecodeInstruction(instr_info);
}

inline rctdl_err_t TrcPktDecodeI::accessMemory(const rctdl_vaddr_t address, const rctdl_mem_space_acc_t mem_space, uint32_t *num_bytes, uint8_t *p_buffer)
{
    return m_mem_access.first()->ReadTargetMemory(address,getCoreSightTraceID(),mem_space, num_bytes,p_buffer);
}


/**********************************************************************/

template <class P, class Pc>
class TrcPktDecodeBase : public TrcPktDecodeI, public IPktDataIn<P>
{
public:
    TrcPktDecodeBase(const char *component_name);
    TrcPktDecodeBase(const char *component_name, int instIDNum);
    virtual ~TrcPktDecodeBase();

    virtual rctdl_datapath_resp_t PacketDataIn( const rctdl_datapath_op_t op,
                                                const rctdl_trc_index_t index_sop,
                                                const P *p_packet_in);
    

    /* protocol configuration */
    rctdl_err_t setProtocolConfig(Pc *config); 
    const Pc *  getProtocolConfig() const { return  m_config; };
    
protected:
    void ClearConfigObj();

    /* the protocol configuration */
    Pc *          m_config;
    /* the current input packet */
    const P *     m_curr_packet_in;
    
};


template <class P, class Pc> TrcPktDecodeBase<P, Pc>::TrcPktDecodeBase(const char *component_name) : 
    TrcPktDecodeI(component_name),
    m_config(0)
{
}

template <class P, class Pc> TrcPktDecodeBase<P, Pc>::TrcPktDecodeBase(const char *component_name, int instIDNum) : 
    TrcPktDecodeI(component_name,instIDNum),
    m_config(0)
{
}

template <class P, class Pc> TrcPktDecodeBase<P, Pc>::~TrcPktDecodeBase() 
{
    ClearConfigObj();
}

template <class P, class Pc> rctdl_datapath_resp_t TrcPktDecodeBase<P, Pc>::PacketDataIn( const rctdl_datapath_op_t op,
                                                const rctdl_trc_index_t index_sop,
                                                const P *p_packet_in)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_CONT;
    if(!checkInit())
    {
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_NOT_INIT));
        return RCTDL_RESP_FATAL_NOT_INIT;
    }

    switch(op)
    {
    case RCTDL_OP_DATA:
        if(p_packet_in == 0)
        {
            LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_INVALID_PARAM_VAL));
            resp = RCTDL_RESP_FATAL_INVALID_PARAM;
        }
        else
        {
            m_curr_packet_in = p_packet_in;
            m_index_curr_pkt = index_sop;
            resp = processPacket();
        }
        break;

    case RCTDL_OP_EOT:
        resp = onEOT();
        break;

    case RCTDL_OP_FLUSH:
        resp = onFlush();
        break;

    case RCTDL_OP_RESET:
        resp = onReset();
        break;

    default:
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_INVALID_PARAM_VAL));
        resp = RCTDL_RESP_FATAL_INVALID_OP;
        break;
    }
    return resp;
}

    /* protocol configuration */
template <class P, class Pc>  rctdl_err_t TrcPktDecodeBase<P, Pc>::setProtocolConfig(Pc *config)
{
    rctdl_err_t err = RCTDL_ERR_INVALID_PARAM_VAL;
    if(config != 0)
    {
        ClearConfigObj(); // remove any current config
        m_config = new (std::nothrow) Pc(*config); // make a copy of the config - don't rely on the object passed in being valid outside the context of the call.
        if(m_config != 0)
        {
            err = onProtocolConfig();
            if(err == RCTDL_OK)
                m_config_init_ok = true;
        }
        else
            err = RCTDL_ERR_MEM;
    }
    return err;
}

template <class P, class Pc> void TrcPktDecodeBase<P, Pc>::ClearConfigObj()
{
    if(m_config)
    {
        delete m_config;
        m_config = 0;
    }
}

/** @}*/
#endif // ARM_TRC_PKT_DECODE_BASE_H_INCLUDED

/* End of File trc_pkt_decode_base.h */

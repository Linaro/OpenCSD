/*
 * \file       rctdl_dcd_tree_elem.h
 * \brief      Reference CoreSight Trace Decoder : Decode tree element.
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


#ifndef ARM_RCTDL_DCD_TREE_ELEM_H_INCLUDED
#define ARM_RCTDL_DCD_TREE_ELEM_H_INCLUDED


/** @class decoder_elements 
 *  @brief Decode tree element base structure.
 *  @addtogroup dcd_tree
 *
 *  Element describes the protocol supported for this element and 
 *  contains pointers to packet processor and decoder for the protocol.
 * 
 *  Union of all recognised decoders, plus an attachment point for an external decoder.
 */
typedef struct _decoder_elements 
{
    union {
        struct {
            TrcPktProcEtmV3 *proc;
            void * /*TrcPktDecodeEtmV3 * */ dcd; //** TBD
        } etmv3;    /**< ETMv3 decoders */
        struct {
            TrcPktProcEtmV4I *proc;
            void * /*TrcPktDecodeEtmV4 **/ dcd; //** TBD
        } etmv4i;   /**< ETMv4 Decoders */
        struct {
            TrcPktProcEtmV4D *proc;
            void * /*TrcPktDecodeEtmV4 **/ dcd; //** TBD
        } etmv4d;
        struct {
            TrcPktProcPtm *proc;
            void * /*TrcPktDecodePtm **/ dcd; //** TBD
        } ptm;
        struct {
            void * proc;
            void * dcd;
        } extern_custom;
    } decoder;
    rctdl_trace_protocol_t protocol;
    bool created;  /**< decode tree created this element (destroy it on tree destruction) */
} decoder_element;


/*!
 *  @class DecodeTreeElement   
 *  @brief Decode tree element
 *  @addtogroup dcd_tree
 *
 *   
 */
class DecodeTreeElement : public decoder_element
{
public:
    DecodeTreeElement();
    ~DecodeTreeElement() {};

    void SetProcElement(const rctdl_trace_protocol_t elem_type, void *pkt_proc, const bool created);
    void SetDecoderElement(void *pkt_decode);
    void DestroyElem();

    TrcPktProcEtmV3 *  getEtmV3PktProc() const
    {
        if(protocol == RCTDL_PROTOCOL_ETMV3)
            return decoder.etmv3.proc;
        return 0;
    }

    TrcPktProcEtmV4I *  getEtmV4IPktProc() const
    {
        if(protocol == RCTDL_PROTOCOL_ETMV4I)
            return decoder.etmv4i.proc;
        return 0;
    }

    TrcPktProcEtmV4D *  getEtmV4DPktProc() const
    {
        if(protocol == RCTDL_PROTOCOL_ETMV4D)
            return decoder.etmv4d.proc;
        return 0;
    }
    TrcPktProcPtm *     getPtmPktProc() const
    {
        if(protocol == RCTDL_PROTOCOL_PTM)
            return decoder.ptm.proc;
        return 0;
    }

    void  *             getExternPktProc() const
    {
        if(protocol == RCTDL_PROTOCOL_EXTERN)
            return decoder.extern_custom.proc;
        return 0;
    }

    rctdl_trace_protocol_t getProtocol() const { return protocol; };

};

inline DecodeTreeElement::DecodeTreeElement()
{
    // set one element in union to zero all.
    decoder.extern_custom.dcd = 0;
    decoder.extern_custom.proc = 0;
    protocol = RCTDL_PROTOCOL_END;  
    created = false;
}

#endif // ARM_RCTDL_DCD_TREE_ELEM_H_INCLUDED

/* End of File rctdl_dcd_tree_elem.h */

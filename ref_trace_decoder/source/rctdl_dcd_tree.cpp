/*
 * \file       rctdl_dcd_tree.cpp
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

#include "rctdl_dcd_tree.h"


void DecodeTreeElement::SetProcElement(const rctdl_trace_protocol_t protocol_type, void *pkt_proc, const bool decoder_created)
{
     created = decoder_created;
     protocol = protocol_type;

     // as pointers are in a union, assume types OK and just set the extern void types.
     decoder.extern_custom.proc = pkt_proc;

}
 
void DecodeTreeElement::SetDecoderElement(void *pkt_decode)
{
    decoder.extern_custom.dcd = pkt_decode;
}
// destroy the elements using correctly typed pointers to ensure destructors are called.
void DecodeTreeElement::DestroyElem()
{
    if(created)
    {
        switch(protocol)
        {
        case RCTDL_PROTOCOL_ETMV3:
            delete decoder.etmv3.proc;
            decoder.etmv3.proc = 0;
            delete decoder.etmv3.dcd;
            decoder.etmv3.dcd = 0;
            break;

        case RCTDL_PROTOCOL_ETMV4I:
            delete decoder.etmv4i.proc;
            decoder.etmv4i.proc = 0;
            delete decoder.etmv4i.dcd;
            decoder.etmv4i.dcd = 0;
            break;

        case RCTDL_PROTOCOL_ETMV4D:
            delete decoder.etmv4d.proc;
            decoder.etmv4d.proc = 0;
            delete decoder.etmv4d.dcd;
            decoder.etmv4d.dcd = 0;
            break;

        case RCTDL_PROTOCOL_PTM:
            delete decoder.ptm.proc;
            decoder.ptm.proc = 0;
            delete decoder.ptm.dcd;
            decoder.ptm.dcd  = 0;
            break;
        }
    }
}


/***************************************************************/
ITraceErrorLog *DecodeTree::s_i_error_logger = &DecodeTree::s_error_logger;
std::vector<DecodeTree *> DecodeTree::s_trace_dcd_trees;
rctdlDefaultErrorLogger DecodeTree::s_error_logger;

DecodeTree *DecodeTree::CreateDecodeTree(const rctdl_dcd_tree_src_t src_type, uint32_t formatterCfgFlags)
{
    DecodeTree *dcd_tree = new (std::nothrow) DecodeTree();
    if(dcd_tree != 0)
    {
        if(dcd_tree->initialise(src_type, formatterCfgFlags))
        {
            s_trace_dcd_trees.push_back(dcd_tree);
        }
        else 
        {
            delete dcd_tree;
            dcd_tree = 0;
        }
    }
    return dcd_tree;
}

void DecodeTree::DestroyDecodeTree(DecodeTree *p_dcd_tree)
{
}

void DecodeTree::setAlternateErrorLogger(ITraceErrorLog *p_error_logger)
{
    if(p_error_logger)
        s_i_error_logger = p_error_logger;
    else
        s_i_error_logger = &s_error_logger;
}

/***************************************************************/

DecodeTree::DecodeTree() :
    m_i_instr_decode(0),
    m_i_mem_access(0),
    m_i_gen_elem_out(0),
    m_i_decoder_root(0),
    m_frame_deformatter_root(0),
    m_decode_elem_iter(0)

{
    for(int i = 0; i < 0x80; i++)
        m_decode_elements[i] = 0;
}

DecodeTree::~DecodeTree()
{
    for(int i = 0; i < 0x80; i++)
    {
        destroyDecodeElement(i);
    }
}

rctdl_datapath_resp_t DecodeTree::TraceDataIn( const rctdl_datapath_op_t op,
                                               const rctdl_trc_index_t index,
                                               const uint32_t dataBlockSize,
                                               const uint8_t *pDataBlock,
                                               uint32_t *numBytesProcessed)
{
    if(m_i_decoder_root)
        return m_i_decoder_root->TraceDataIn(op,index,dataBlockSize,pDataBlock,numBytesProcessed);
    *numBytesProcessed = 0;
    return RCTDL_RESP_FATAL_NOT_INIT;
}

/* set key interfaces - attach / replace on any existing tree components */
void DecodeTree::setInstrDecoder(IInstrDecode *i_instr_decode)
{
    //** TBD - implement for first decoder
}

void DecodeTree::setMemAccessor(ITargetMemAccess *i_mem_access)
{
    //** TBD - implement for first decoder
}

void DecodeTree::setGenTraceElemOutI(ITrcGenElemIn *i_gen_trace_elem)
{
   //** TBD - implement for first decoder
}


/* create packet processing element only - attach to CSID in config */
rctdl_err_t DecodeTree::createETMv3PktProcessor(const EtmV3Config *p_config)
{
    rctdl_err_t err = RCTDL_OK;
#if 0
    uint8_t CSID = 0;   // default for single stream decoder (no deformatter) - we ignore the ID
    if(usingFormatter())
        CSID = p_config->getTraceID();

    // see if we can attach to the desire CSID point
    if((err = createDecodeElement(CSID)) == RCTDL_OK)
    {

        TrcPktProcEtmV3 *pProc = new (std::nothrow) TrcPktProcEtmV3(CSID);
        if(pProc)
        {
            err = pProc->setProtocolConfig(p_config);
            if(err == RCTDL_OK)
                pProc->getErrorLogAttachPt()->attach(DecodeTree::s_i_error_logger);

            if(usingFormatter() && (err == RCTDL_OK))
                err = m_frame_deformatter_root->getIDStreamAttachPt(p_config->getTraceID())->attach(pProc);            

            if(err != RCTDL_OK)
            {
                // unable to attach as in use
                delete pProc;
                destroyDecodeElement(CSID);
            }
            else
            {
                if(!usingFormatter())
                    setSingleRoot(pProc);
                m_decode_elements[CSID]->decoder.etmv3.proc = pProc;
                m_decode_elements[CSID]->created = true;
                m_decode_elements[CSID]->protocol = RCTDL_PROTOCOL_ETMV3;
            }
        }
        else
            err = RCTDL_ERR_MEM;
    }
#endif
    return err;
}

rctdl_err_t DecodeTree::createETMv4PktProcessor(const EtmV4Config *p_config, bool bDataChannel /*= false*/)
{
    rctdl_err_t err = RCTDL_ERR_NOT_INIT;
    uint8_t CSID = 0;   // default for single stream decoder (no deformatter) - we ignore the ID
    if(usingFormatter())
        CSID = p_config->getTraceID();

    // see if we can attach to the desired CSID point
    if((err = createDecodeElement(CSID)) == RCTDL_OK)
    {
        TrcPktProcI *pProc = 0;
        if(!bDataChannel)
        {
            pProc = new (std::nothrow) TrcPktProcEtmV4I(CSID);
            if(pProc)
                err = static_cast<TrcPktProcEtmV4I *>(pProc)->setProtocolConfig(p_config);
            else
                err = RCTDL_ERR_MEM;
        }
        else
        {
#if 0
            pProc = new (std::nothrow) TrcPktProcEtmV4D(CSID);
            if(pProc)
                err = static_cast<TrcPktProcEtmV4D *>(pProc)->setProtocolConfig(p_config);
            else
                err = RCTDL_ERR_MEM;
#endif
        }

        if(pProc)
        {
            err = pProc->getErrorLogAttachPt()->attach(DecodeTree::s_i_error_logger);

            if(usingFormatter() && (err == RCTDL_OK))
                err = m_frame_deformatter_root->getIDStreamAttachPt(p_config->getTraceID())->attach(pProc);            

            if(err != RCTDL_OK)
            {
                // unable to attach as in use
                delete pProc;
                destroyDecodeElement(CSID);
            }
            else
            {
                if(!usingFormatter())
                    setSingleRoot(pProc);
                m_decode_elements[CSID]->SetProcElement( bDataChannel ? RCTDL_PROTOCOL_ETMV4D : RCTDL_PROTOCOL_ETMV4I,pProc,true);
            }
        }
    }    
   return err;
}

rctdl_err_t DecodeTree::createPTMPktProcessor(const PtmConfig *p_config)
{
    rctdl_err_t err = RCTDL_ERR_NOT_INIT;
        //** TBD
    return err;
}

/* create full decoder - packet processor + packet decoder  - attach to CSID in config */
rctdl_err_t DecodeTree::createETMv3Decoder(const EtmV3Config *p_config)
{
    rctdl_err_t err = RCTDL_ERR_NOT_INIT;
    uint8_t CSID = 0;   // default for single stream decoder (no deformatter) - we ignore the ID
    if(usingFormatter())
        CSID = p_config->getTraceID();
    /* err = */ createETMv3PktProcessor(p_config);
    if(err == RCTDL_OK) // created the packet processor and the decoder element*/
    {
            //** TBD
    }
    return err;
}

rctdl_err_t DecodeTree::createETMv4Decoder(const EtmV4Config *p_config, bool bDataChannel /*= false*/)
{
    rctdl_err_t err = RCTDL_ERR_NOT_INIT;
        //** TBD
    return err;
}

rctdl_err_t DecodeTree::createPTMDecoder(const PtmConfig *p_config)
{
    rctdl_err_t err = RCTDL_ERR_NOT_INIT;
        //** TBD
    return err;
}


rctdl_err_t DecodeTree::removeDecoder(const uint8_t CSID)
{
    rctdl_err_t err = RCTDL_OK;
    uint8_t localID = CSID;
    if(!usingFormatter())
        localID = 0;

    if(usingFormatter() && !RCTDL_IS_VALID_CS_SRC_ID(CSID))
        err = RCTDL_ERR_INVALID_ID;
    else
    {
        destroyDecodeElement(localID);
    }
    return err;
}

DecodeTreeElement * DecodeTree::getDecoderElement(const uint8_t CSID) const
{
    DecodeTreeElement *ret_elem = 0;
    if(usingFormatter() && RCTDL_IS_VALID_CS_SRC_ID(CSID))
    {
        ret_elem = m_decode_elements[CSID]; 
    }
    else
        ret_elem = m_decode_elements[0];    // ID 0 is used if single leaf tree.
    return ret_elem;
}

DecodeTreeElement *DecodeTree::getFirstElement(uint8_t &elemID)
{
    m_decode_elem_iter = 0;
    return getNextElement(elemID);
}

DecodeTreeElement *DecodeTree::getNextElement(uint8_t &elemID)
{
    DecodeTreeElement *ret_elem = 0;

    if(m_decode_elem_iter < 0x80)
    {
        // find a none zero entry or end of range
        while((m_decode_elements[m_decode_elem_iter] == 0) && (m_decode_elem_iter < 0x80))
            m_decode_elem_iter++;

        // return entry unless end of range
        if(m_decode_elem_iter < 0x80)
        {
            ret_elem = m_decode_elements[m_decode_elem_iter];
            elemID = m_decode_elem_iter;
            m_decode_elem_iter++;
        }
    }
    return ret_elem;
}

bool DecodeTree::initialise(const rctdl_dcd_tree_src_t type, uint32_t formatterCfgFlags)
{
    bool initOK = true;
    m_dcd_tree_type = type;
    if(type ==  RCTDL_TRC_SRC_FRAME_FORMATTED)
    {
        // frame formatted - we want to create the deformatter and hook it up
        m_frame_deformatter_root = new (std::nothrow) TraceFormatterFrameDecoder();
        if(m_frame_deformatter_root)
        {
            m_frame_deformatter_root->Configure(formatterCfgFlags);
            m_frame_deformatter_root->getErrLogAttachPt()->attach(DecodeTree::s_i_error_logger);
            m_i_decoder_root = dynamic_cast<ITrcDataIn*>(m_frame_deformatter_root);
        }
        else 
            initOK = false;
    }
    return initOK;
}

void DecodeTree::setSingleRoot(TrcPktProcI *pComp)
{
    m_i_decoder_root = static_cast<ITrcDataIn*>(pComp);
}

rctdl_err_t DecodeTree::createDecodeElement(const uint8_t CSID)
{
    rctdl_err_t err = RCTDL_ERR_INVALID_ID;
    if(CSID < 0x80)
    {
        if(m_decode_elements[CSID] == 0)
        {
            m_decode_elements[CSID] = new (std::nothrow) DecodeTreeElement();
            if(m_decode_elements[CSID] == 0)
                err = RCTDL_ERR_MEM;
            else 
                err = RCTDL_OK;
        }
        else
            err = RCTDL_ERR_ATTACH_TOO_MANY;
    }
    return err;
}

void DecodeTree::destroyDecodeElement(const uint8_t CSID)
{
    if(CSID < 0x80)
    {
        if(m_decode_elements[CSID] != 0)
        {
            m_decode_elements[CSID]->DestroyElem();
            delete m_decode_elements[CSID];
            m_decode_elements[CSID] = 0;
        }
    }
}

/* End of File rctdl_dcd_tree.cpp */

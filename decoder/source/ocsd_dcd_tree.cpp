/*
 * \file       ocsd_dcd_tree.cpp
 * \brief      OpenCSD : 
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

#include "common/ocsd_dcd_tree.h"
#include "common/ocsd_lib_dcd_register.h"
#include "mem_acc/trc_mem_acc_mapper.h"

/***************************************************************/
ITraceErrorLog *DecodeTree::s_i_error_logger = &DecodeTree::s_error_logger; 
std::list<DecodeTree *> DecodeTree::s_trace_dcd_trees;  /**< list of pointers to decode tree objects */
ocsdDefaultErrorLogger DecodeTree::s_error_logger;     /**< The library default error logger */
TrcIDecode DecodeTree::s_instruction_decoder;           /**< default instruction decode library */

DecodeTree *DecodeTree::CreateDecodeTree(const ocsd_dcd_tree_src_t src_type, uint32_t formatterCfgFlags)
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
    std::list<DecodeTree *>::iterator it;
    bool bDestroyed = false;
    it = s_trace_dcd_trees.begin();
    while(!bDestroyed && (it != s_trace_dcd_trees.end()))
    {
        if(*it == p_dcd_tree)
        {
            s_trace_dcd_trees.erase(it);
            delete p_dcd_tree;
            bDestroyed = true;
        }
        else
            it++;
    }
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
    m_i_instr_decode(&s_instruction_decoder),
    m_i_mem_access(0),
    m_i_gen_elem_out(0),
    m_i_decoder_root(0),
    m_frame_deformatter_root(0),
    m_decode_elem_iter(0),
    m_default_mapper(0)

{
    for(int i = 0; i < 0x80; i++)
        m_decode_elements[i] = 0;
}

DecodeTree::~DecodeTree()
{
    for(uint8_t i = 0; i < 0x80; i++)
    {
        destroyDecodeElement(i);
    }

}



ocsd_datapath_resp_t DecodeTree::TraceDataIn( const ocsd_datapath_op_t op,
                                               const ocsd_trc_index_t index,
                                               const uint32_t dataBlockSize,
                                               const uint8_t *pDataBlock,
                                               uint32_t *numBytesProcessed)
{
    if(m_i_decoder_root)
        return m_i_decoder_root->TraceDataIn(op,index,dataBlockSize,pDataBlock,numBytesProcessed);
    *numBytesProcessed = 0;
    return OCSD_RESP_FATAL_NOT_INIT;
}

/* set key interfaces - attach / replace on any existing tree components */
void DecodeTree::setInstrDecoder(IInstrDecode *i_instr_decode)
{
    uint8_t elemID;
    DecodeTreeElement *pElem = 0;

    pElem = getFirstElement(elemID);
    while(pElem != 0)
    {
        pElem->getDecoderMngr()->attachInstrDecoder(pElem->getDecoderHandle(),i_instr_decode);
        pElem = getNextElement(elemID);
    }
}

void DecodeTree::setMemAccessI(ITargetMemAccess *i_mem_access)
{
    uint8_t elemID;
    DecodeTreeElement *pElem = 0;
   
    pElem = getFirstElement(elemID);
    while(pElem != 0)
    {
        pElem->getDecoderMngr()->attachMemAccessor(pElem->getDecoderHandle(),i_mem_access);
        pElem = getNextElement(elemID);
    }
    m_i_mem_access = i_mem_access;
}

void DecodeTree::setGenTraceElemOutI(ITrcGenElemIn *i_gen_trace_elem)
{
    uint8_t elemID;
    DecodeTreeElement *pElem = 0;

    pElem = getFirstElement(elemID);
    while(pElem != 0)
    {
        pElem->getDecoderMngr()->attachOutputSink(pElem->getDecoderHandle(),i_gen_trace_elem);
        pElem = getNextElement(elemID);
    }
}

ocsd_err_t DecodeTree::createMemAccMapper(memacc_mapper_t type)
{
    // clean up any old one
    destroyMemAccMapper();

    // make a new one
    switch(type)
    {
    default:
    case MEMACC_MAP_GLOBAL:
        m_default_mapper = new (std::nothrow) TrcMemAccMapGlobalSpace();
        break;
    }

    // set the access interface
    if(m_default_mapper)
    {
        setMemAccessI(m_default_mapper);
        m_default_mapper->setErrorLog(s_i_error_logger);
    }

    return (m_default_mapper != 0) ? OCSD_OK : OCSD_ERR_MEM;
}

ocsd_err_t DecodeTree::addMemAccessorToMap(TrcMemAccessorBase *p_accessor, const uint8_t cs_trace_id)
{
    ocsd_err_t err= OCSD_ERR_NOT_INIT;
    if(m_default_mapper)
        err = m_default_mapper->AddAccessor(p_accessor,cs_trace_id);
    return err;
}

void DecodeTree::destroyMemAccMapper()
{
    if(m_default_mapper)
    {
        m_default_mapper->RemoveAllAccessors();
        delete m_default_mapper;
        m_default_mapper = 0;
    }
}

ocsd_err_t DecodeTree::removeMemAccessor(TrcMemAccessorBase *p_accessor)
{
    ocsd_err_t err= OCSD_ERR_NOT_INIT;
    if(m_default_mapper)
    {
        err = m_default_mapper->RemoveAccessor(p_accessor);
    }
    return err;
}

ocsd_err_t DecodeTree::removeMemAccessorByAddress(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t cs_trace_id)
{
    ocsd_err_t err= OCSD_ERR_NOT_INIT;
    if(m_default_mapper)
    {
        err = m_default_mapper->RemoveAccessorByAddress(address,mem_space,cs_trace_id);
    }
    return err;
}

void DecodeTree::logMappedRanges()
{
    if(m_default_mapper)
        m_default_mapper->logMappedRanges();
}

ocsd_err_t DecodeTree::createDecoder(const std::string &decoderName, const int createFlags, const CSConfig *pConfig)
{
    ocsd_err_t err = OCSD_OK;
    IDecoderMngr *pDecoderMngr = 0;
    TraceComponent *pTraceComp = 0;
    int crtFlags = createFlags;

    uint8_t CSID = 0;   // default for single stream decoder (no deformatter) - we ignore the ID
    if(usingFormatter())
    {
        CSID = pConfig->getTraceID();
        crtFlags |= OCSD_CREATE_FLG_INST_ID;
    }

    // create the decode element to attach to the channel.
    if((err = createDecodeElement(CSID)) != OCSD_OK)
        return err;

    // get the libary decoder register.
    OcsdLibDcdRegister * lib_reg = OcsdLibDcdRegister::getDecoderRegister();
    if(lib_reg == 0)
        return OCSD_ERR_NOT_INIT;

    // find the named decoder
    if((err = lib_reg->getDecoderMngrByName(decoderName,&pDecoderMngr)) != OCSD_OK)
        return err;

    // got the decoder...
    if((err = pDecoderMngr->createDecoder(crtFlags,(int)CSID,pConfig,&pTraceComp)) != OCSD_OK)
        return err;

    m_decode_elements[CSID]->SetDecoderElement(decoderName, pDecoderMngr, pTraceComp, true);

    // always attach an error logger
    if(err == OCSD_OK)
        err = pDecoderMngr->attachErrorLogger(pTraceComp,DecodeTree::s_i_error_logger);

    // if we created a packet decoder it may need additional components.
    if(crtFlags &  OCSD_CREATE_FLG_FULL_DECODER)
    {
        if(m_i_instr_decode && (err == OCSD_OK))
            err = pDecoderMngr->attachInstrDecoder(pTraceComp,m_i_instr_decode);
        
        if(err == OCSD_ERR_DCD_INTERFACE_UNUSED)    // ignore if instruction decoder refused
            err = OCSD_OK;

        if(m_i_mem_access && (err == OCSD_OK))
            err = pDecoderMngr->attachMemAccessor(pTraceComp,m_i_mem_access);

        if(err == OCSD_ERR_DCD_INTERFACE_UNUSED)    // ignore if mem accessor refused
            err = OCSD_OK;

        if( m_i_gen_elem_out && (err == OCSD_OK))
            err = pDecoderMngr->attachOutputSink(pTraceComp,m_i_gen_elem_out);
    }

    // finally attach the packet processor input to the demux output channel
    if(err == OCSD_OK)
    {
        ITrcDataIn *pDataIn = 0;
        if((err = pDecoderMngr->getDataInputI(pTraceComp,&pDataIn)) == OCSD_OK)
        {
            // got the interface -> attach to demux, or direct to input of decode tree
            if(usingFormatter())
                err = m_frame_deformatter_root->getIDStreamAttachPt(CSID)->attach(pDataIn);
            else
                m_i_decoder_root = pDataIn;
        }
    }

    if(err != OCSD_OK)
    {
        destroyDecodeElement(CSID); // will destroy decoder as well.       
    }
    return err;
}

ocsd_err_t DecodeTree::removeDecoder(const uint8_t CSID)
{
    ocsd_err_t err = OCSD_OK;
    uint8_t localID = CSID;
    if(!usingFormatter())
        localID = 0;

    if(usingFormatter() && !OCSD_IS_VALID_CS_SRC_ID(CSID))
        err = OCSD_ERR_INVALID_ID;
    else
    {
        destroyDecodeElement(localID);
    }
    return err;
}

DecodeTreeElement * DecodeTree::getDecoderElement(const uint8_t CSID) const
{
    DecodeTreeElement *ret_elem = 0;
    if(usingFormatter() && OCSD_IS_VALID_CS_SRC_ID(CSID))
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

bool DecodeTree::initialise(const ocsd_dcd_tree_src_t type, uint32_t formatterCfgFlags)
{
    bool initOK = true;
    m_dcd_tree_type = type;
    if(type ==  OCSD_TRC_SRC_FRAME_FORMATTED)
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

ocsd_err_t DecodeTree::createDecodeElement(const uint8_t CSID)
{
    ocsd_err_t err = OCSD_ERR_INVALID_ID;
    if(CSID < 0x80)
    {
        if(m_decode_elements[CSID] == 0)
        {
            m_decode_elements[CSID] = new (std::nothrow) DecodeTreeElement();
            if(m_decode_elements[CSID] == 0)
                err = OCSD_ERR_MEM;
            else 
                err = OCSD_OK;
        }
        else
            err = OCSD_ERR_ATTACH_TOO_MANY;
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

ocsd_err_t DecodeTree::setIDFilter(std::vector<uint8_t> &ids)
{
    ocsd_err_t err = OCSD_ERR_DCDT_NO_FORMATTER;
    if(usingFormatter())
    {
        err = m_frame_deformatter_root->OutputFilterAllIDs(false);
        if(err == OCSD_OK)
            err = m_frame_deformatter_root->OutputFilterIDs(ids,true);
    }
    return err;
}

ocsd_err_t DecodeTree::clearIDFilter()
{
    ocsd_err_t err = OCSD_ERR_DCDT_NO_FORMATTER;
    if(usingFormatter())
    {
        err = m_frame_deformatter_root->OutputFilterAllIDs(true);
    }
    return err;
}

/* End of File ocsd_dcd_tree.cpp */

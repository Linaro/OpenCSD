/*
 * \file       ocsd_c_api_custom_obj.cpp
 * \brief      OpenCSD : 
 * 
 * \copyright  Copyright (c) 2016, ARM Limited. All Rights Reserved.
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

/* pull in the C++ decode library */
#include "opencsd.h"

#include "c_api/opencsd_c_api.h"
#include "ocsd_c_api_custom_obj.h"
#include "common/ocsd_lib_dcd_register.h"

/***************** C-API functions ********************************/

/** register a custom decoder with the library */
OCSD_C_API ocsd_err_t ocsd_register_custom_decoder(const char *name, ocsd_extern_dcd_fact_t  *p_dcd_fact)
{
    ocsd_err_t err = OCSD_OK;
    OcsdLibDcdRegister *pRegister = OcsdLibDcdRegister::getDecoderRegister();
    
    // check not already registered
    if(pRegister->isRegisteredDecoder(name))
        return OCSD_ERR_DCDREG_NAME_REPEAT;

    // validate the factory interface structure
    if((p_dcd_fact->createDecoder == 0) || 
       (p_dcd_fact->destroyDecoder == 0) ||
       (p_dcd_fact->csidFromConfig == 0)
       )
        return OCSD_ERR_INVALID_PARAM_VAL;
    
    // create a wrapper.
    CustomDcdMngrWrapper *pWrapper = new (std::nothrow) CustomDcdMngrWrapper();
    if(pRegister == 0)
        return OCSD_ERR_MEM;

    p_dcd_fact->protocol_id = OcsdLibDcdRegister::getNextCustomProtocolID();
    if(p_dcd_fact->protocol_id < OCSD_PROTOCOL_END)
    {
        // fill out the wrapper and register it
        pWrapper->setAPIDcdFact(p_dcd_fact);
        err = pRegister->registerDecoderTypeByName(name,pWrapper);
        if(err != OCSD_OK)
            OcsdLibDcdRegister::releaseLastCustomProtocolID();
    }
    else
        err =  OCSD_ERR_DCDREG_TOOMANY; // too many decoders
    
    if(err != OCSD_OK)
        delete pWrapper;

    return err;
}

OCSD_C_API ocsd_err_t ocsd_deregister_decoders()
{   
    // destroys all builtin and custom decoders & library registration object.
    OcsdLibDcdRegister::deregisterAllDecoders();    
    return OCSD_OK;
}

/***************** Decode Manager Wrapper *****************************/

CustomDcdMngrWrapper::CustomDcdMngrWrapper()
{
    m_dcd_fact.protocol_id = OCSD_PROTOCOL_END;    
}


    // set the C-API decoder factory interface
void CustomDcdMngrWrapper::setAPIDcdFact(ocsd_extern_dcd_fact_t *p_dcd_fact)
{
    m_dcd_fact = *p_dcd_fact;
}

// create and destroy decoders
ocsd_err_t CustomDcdMngrWrapper::createDecoder(const int create_flags, const int instID, const CSConfig *p_config,  TraceComponent **ppComponent)
{
    ocsd_err_t err = OCSD_OK;
    if(m_dcd_fact.protocol_id == OCSD_PROTOCOL_END)
        return OCSD_ERR_NOT_INIT;

    ocsd_extern_dcd_inst_t dcd_inst;
    err = m_dcd_fact.createDecoder( create_flags,
                                    ((CustomConfigWrapper *)p_config)->getConfig(),
                                    &dcd_inst);
    if(err == OCSD_OK)
    {
        CustomDecoderWrapper *pComp = new (std::nothrow) CustomDecoderWrapper(dcd_inst,dcd_inst.p_decoder_name,dcd_inst.cs_id);
        *ppComponent = pComp;
        if(pComp == 0)
            err = OCSD_ERR_MEM;
    }
    return err;
}

ocsd_err_t CustomDcdMngrWrapper::destroyDecoder(TraceComponent *pComponent)
{
    CustomDecoderWrapper *pCustWrap = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(m_dcd_fact.protocol_id != OCSD_PROTOCOL_END)
        m_dcd_fact.destroyDecoder(pCustWrap->getDecoderInstInfo()->decoder_handle);
    return OCSD_OK;
}

const ocsd_trace_protocol_t CustomDcdMngrWrapper::getProtocolType() const
{
    return m_dcd_fact.protocol_id;
}

ocsd_err_t CustomDcdMngrWrapper::createConfigFromDataStruct(CSConfig **pConfigBase, const void *pDataStruct)
{
    ocsd_err_t err = OCSD_OK;
    CustomConfigWrapper *pConfig = new (std::nothrow) CustomConfigWrapper(pDataStruct);
    if(!pConfig)
        return OCSD_ERR_MEM;

    if(m_dcd_fact.csidFromConfig == 0)
        return OCSD_ERR_NOT_INIT;

    unsigned char csid;
    err = m_dcd_fact.csidFromConfig(pDataStruct,&csid);
    if(err == OCSD_OK)
    {
        pConfig->setCSID(csid);        
        *pConfigBase = pConfig;
    }
    return err;
}

ocsd_err_t CustomDcdMngrWrapper::getDataInputI(TraceComponent *pComponent, ITrcDataIn **ppDataIn)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;

    *ppDataIn = pDecoder;
    return OCSD_OK;
}

// component connections
// all
ocsd_err_t CustomDcdMngrWrapper::attachErrorLogger(TraceComponent *pComponent, ITraceErrorLog *pIErrorLog)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;
}

// full decoder
ocsd_err_t CustomDcdMngrWrapper::attachInstrDecoder(TraceComponent *pComponent, IInstrDecode *pIInstrDec)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;

    return OCSD_OK; 
}

ocsd_err_t CustomDcdMngrWrapper::attachMemAccessor(TraceComponent *pComponent, ITargetMemAccess *pMemAccessor)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;
}

ocsd_err_t CustomDcdMngrWrapper::attachOutputSink(TraceComponent *pComponent, ITrcGenElemIn *pOutSink)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;
}

// pkt processor only
ocsd_err_t CustomDcdMngrWrapper::attachPktMonitor(TraceComponent *pComponent, ITrcTypedBase *pPktRawDataMon)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;
}

ocsd_err_t CustomDcdMngrWrapper::attachPktIndexer(TraceComponent *pComponent, ITrcTypedBase *pPktIndexer)
{
    // indexers for external custom will also be external and custom.
    return OCSD_ERR_DCD_INTERFACE_UNUSED;
}

ocsd_err_t CustomDcdMngrWrapper::attachPktSink(TraceComponent *pComponent, ITrcTypedBase *pPktDataInSink)
{
    CustomDecoderWrapper *pDecoder = dynamic_cast<CustomDecoderWrapper *>(pComponent);
    if(pDecoder == 0)
        return OCSD_ERR_INVALID_PARAM_TYPE;
}

void CustomDcdMngrWrapper::pktToString(void *pkt, char *pStrBuffer, int bufSize)
{
    if(m_dcd_fact.pktToString)
        m_dcd_fact.pktToString(pkt,pStrBuffer,bufSize);
}


/************************** Decoder instance wrapper **************************************/

ocsd_datapath_resp_t GenElemOpCB( const void *cb_context,
                                                const ocsd_trc_index_t index_sop, 
                                                const uint8_t trc_chan_id, 
                                                const ocsd_generic_trace_elem *elem)
{
    if(cb_context && ((CustomDecoderWrapper *)cb_context)->m_pGenElemIn)
        return ((CustomDecoderWrapper *)cb_context)->m_pGenElemIn->TraceElemIn(index_sop,trc_chan_id,*(OcsdTraceElement *)elem);
    return OCSD_RESP_FATAL_NOT_INIT;
}

CustomDecoderWrapper::CustomDecoderWrapper( const ocsd_extern_dcd_inst_t &dcd_inst,
                                            const std::string &name, 
                                            const int instID) 
                                            : TraceComponent(name,instID)
{
    m_pGenElemIn = 0;
    m_decoder_inst = dcd_inst;
}

CustomDecoderWrapper::~CustomDecoderWrapper()
{
}

ocsd_datapath_resp_t CustomDecoderWrapper::TraceDataIn( const ocsd_datapath_op_t op,
                                              const ocsd_trc_index_t index,
                                              const uint32_t dataBlockSize,
                                              const uint8_t *pDataBlock,
                                              uint32_t *numBytesProcessed)
{
    if(m_decoder_inst.fn_data_in)
        return m_decoder_inst.fn_data_in( m_decoder_inst.decoder_handle,
                                          op,
                                          index,
                                          dataBlockSize,
                                          pDataBlock,
                                          numBytesProcessed);
    return OCSD_RESP_FATAL_NOT_INIT;
}

/* End of File ocsd_c_api_custom_obj.cpp */

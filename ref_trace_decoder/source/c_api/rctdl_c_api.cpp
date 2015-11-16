/*
 * \file       rctdl_c_api.cpp
 * \brief      Reference CoreSight Trace Decoder : "C" API libary implementation.
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

#include <cstring>

/* pull in the C++ decode library */
#include "rctdl.h"

/* C-API and wrapper objects */
#include "c_api/rctdl_c_api.h"
#include "rctdl_c_api_obj.h"

/*******************************************************************************/
/* C library data - additional data on top of the C++ library objects                                                             */
/*******************************************************************************/

/* keep a list of interface objects for a decode tree for later disposal */
typedef struct _lib_dt_data_list {
    std::vector<TraceElemCBBase *> cb_objs;
} lib_dt_data_list;

/* map lists to handles */
static std::map<dcd_tree_handle_t, lib_dt_data_list *> s_data_map;

/*******************************************************************************/
/* C API functions                                                             */
/*******************************************************************************/

RCTDL_C_API dcd_tree_handle_t rctdl_create_dcd_tree(const rctdl_dcd_tree_src_t src_type, const uint32_t deformatterCfgFlags)
{
    dcd_tree_handle_t handle = C_API_INVALID_TREE_HANDLE;
    handle = (dcd_tree_handle_t)DecodeTree::CreateDecodeTree(src_type,deformatterCfgFlags); 
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        lib_dt_data_list *pList = new (std::nothrow) lib_dt_data_list;
        if(pList != 0)
        {
            s_data_map.insert(std::pair<dcd_tree_handle_t, lib_dt_data_list *>(handle,pList));
        }
        else
        {
            rctdl_destroy_dcd_tree(handle);
            handle = C_API_INVALID_TREE_HANDLE;
        }
    }
    return handle;
}

RCTDL_C_API void rctdl_destroy_dcd_tree(const dcd_tree_handle_t handle)
{
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        GenTraceElemCBObj * pIf = (GenTraceElemCBObj *)(((DecodeTree *)handle)->getGenTraceElemOutI());
        if(pIf != 0)
            delete pIf;

        /* need to clear any associated callback data. */
        std::map<dcd_tree_handle_t, lib_dt_data_list *>::iterator it;
        it = s_data_map.find(handle);
        if(it != s_data_map.end())
        {
            std::vector<TraceElemCBBase *>::iterator itcb;
            itcb = it->second->cb_objs.begin();
            while(itcb != it->second->cb_objs.end())
            {
                delete *itcb;
                itcb++;
            }
            it->second->cb_objs.clear();
            delete it->second;
        }
        DecodeTree::DestroyDecodeTree((DecodeTree *)handle);
    }
}

RCTDL_C_API rctdl_datapath_resp_t rctdl_dt_process_data(const dcd_tree_handle_t handle,
                                            const rctdl_datapath_op_t op,
                                            const rctdl_trc_index_t index,
                                            const uint32_t dataBlockSize,
                                            const uint8_t *pDataBlock,
                                            uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp =  RCTDL_RESP_FATAL_NOT_INIT;
    if(handle != C_API_INVALID_TREE_HANDLE)
        resp = ((DecodeTree *)handle)->TraceDataIn(op,index,dataBlockSize,pDataBlock,numBytesProcessed);
    return resp;
}

RCTDL_C_API rctdl_err_t rctdl_dt_create_etmv4i_pkt_proc(const dcd_tree_handle_t handle, const void *etmv4_cfg, FnEtmv4IPacketDataIn pPktFn)
{
    rctdl_err_t err = RCTDL_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        EtmV4Config cfg;
        cfg = static_cast<const rctdl_etmv4_cfg *>(etmv4_cfg);
        EtmV4ICBObj *p_CBObj = new (std::nothrow) EtmV4ICBObj(pPktFn);

        if(p_CBObj == 0)
            err =  RCTDL_ERR_MEM;

        if(err == RCTDL_OK)
            err = ((DecodeTree *)handle)->createETMv4IPktProcessor(&cfg,p_CBObj);

        if(err == RCTDL_OK)
        {
            std::map<dcd_tree_handle_t, lib_dt_data_list *>::iterator it;
            it = s_data_map.find(handle);
            if(it != s_data_map.end())
                it->second->cb_objs.push_back(p_CBObj);

        }
        else
            delete p_CBObj;
    }
    else
        err = RCTDL_ERR_INVALID_PARAM_VAL;
    return err;
}

RCTDL_C_API rctdl_err_t rctdl_dt_create_etmv4i_decoder(const dcd_tree_handle_t handle, const void *etmv4_cfg)
{
    rctdl_err_t err = RCTDL_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        EtmV4Config cfg;
        cfg = static_cast<const rctdl_etmv4_cfg *>(etmv4_cfg);
        
        // no need for a spcific CB object here - standard generic elements output used.
        if(err == RCTDL_OK)
            err = ((DecodeTree *)handle)->createETMv4Decoder(&cfg);
    }
    else
        err = RCTDL_ERR_INVALID_PARAM_VAL;
    return err;
}

RCTDL_C_API rctdl_err_t rctdl_dt_attach_etmv4i_pkt_mon(const dcd_tree_handle_t handle, const uint8_t trc_chan_id, FnEtmv4IPktMonDataIn pPktFn)
{
    rctdl_err_t err = RCTDL_OK;
    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        DecodeTree *pDT = static_cast<DecodeTree *>(handle);
        DecodeTreeElement *pDTElem = pDT->getDecoderElement(trc_chan_id);
        if((pDTElem != 0) && (pDTElem->getProtocol() == RCTDL_PROTOCOL_ETMV4I))
        {
            EtmV4IPktMonCBObj *pktMonObj = new (std::nothrow) EtmV4IPktMonCBObj(pPktFn);
            if(pktMonObj != 0)
            {
                pDTElem->getEtmV4IPktProc()->getRawPacketMonAttachPt()->attach(pktMonObj);

                // save object pointer for destruction later.
                std::map<dcd_tree_handle_t, lib_dt_data_list *>::iterator it;
                it = s_data_map.find(handle);
                if(it != s_data_map.end())
                    it->second->cb_objs.push_back(pktMonObj);
            }
            else
                err = RCTDL_ERR_MEM;
        }
        else
            RCTDL_ERR_INVALID_PARAM_VAL; // trace ID not found or not match for element protocol type.
    }
    else
        err = RCTDL_ERR_INVALID_PARAM_VAL;
    return err;
}

RCTDL_C_API rctdl_err_t rctdl_dt_set_gen_elem_outfn(const dcd_tree_handle_t handle, FnTraceElemIn pFn)
{

    GenTraceElemCBObj * pCBObj = new (std::nothrow)GenTraceElemCBObj(pFn);
    if(pCBObj)
    {
        ((DecodeTree *)handle)->setGenTraceElemOutI(pCBObj);
        return RCTDL_OK;
    }
    return RCTDL_ERR_MEM;
}

RCTDL_C_API rctdl_err_t rctdl_def_errlog_init(const rctdl_err_severity_t verbosity, const int create_output_logger)
{
    if(DecodeTree::getDefaultErrorLogger()->initErrorLogger(verbosity,(bool)(create_output_logger != 0)))
        return RCTDL_OK;
    return RCTDL_ERR_NOT_INIT;
}

RCTDL_C_API rctdl_err_t rctdl_def_errlog_config_output(const int output_flags, const char *log_file_name)
{
    rctdlMsgLogger *pLogger = DecodeTree::getDefaultErrorLogger()->getOutputLogger();
    if(pLogger)
    {
        pLogger->setLogOpts(output_flags & C_API_MSGLOGOUT_MASK);
        if(log_file_name != NULL)
        {
            pLogger->setLogFileName(log_file_name);
        }
        return RCTDL_OK;
    }
    return RCTDL_ERR_NOT_INIT;    
}

RCTDL_C_API void rctdl_def_errlog_msgout(const char *msg)
{
    rctdlMsgLogger *pLogger = DecodeTree::getDefaultErrorLogger()->getOutputLogger();
    if(pLogger)
        pLogger->LogMsg(msg);
}


RCTDL_C_API rctdl_err_t rctdl_pkt_str(const rctdl_trace_protocol_t pkt_protocol, void *p_pkt, char *buffer, const int buffer_size)
{
    rctdl_err_t err = RCTDL_OK;
    if((buffer == NULL) || (buffer_size < 2))
        return RCTDL_ERR_INVALID_PARAM_VAL;

    std::string pktStr = "";
    buffer[0] = 0;

    switch(pkt_protocol)
    {
    case RCTDL_PROTOCOL_ETMV4I:
        trcPrintElemToString<EtmV4ITrcPacket,rctdl_etmv4_i_pkt>(static_cast<rctdl_etmv4_i_pkt *>(p_pkt), pktStr);
        //EtmV4ITrcPacket::toString(static_cast<rctdl_etmv4_i_pkt *>(p_pkt), pktStr);
        break;

    case RCTDL_PROTOCOL_ETMV3:
        trcPrintElemToString<EtmV3TrcPacket,rctdl_etmv3_pkt>(static_cast<rctdl_etmv3_pkt *>(p_pkt), pktStr);
        break;

    case RCTDL_PROTOCOL_STM:
        trcPrintElemToString<StmTrcPacket,rctdl_stm_pkt>(static_cast<rctdl_stm_pkt *>(p_pkt), pktStr);
        break;

    default:
        err = RCTDL_ERR_NO_PROTOCOL;
        break;
    }

    if(pktStr.size() > 0)
    {
        strncpy(buffer,pktStr.c_str(),buffer_size-1);
        buffer[buffer_size-1] = 0;
    }
    return err;
}

RCTDL_C_API rctdl_err_t rctdl_gen_elem_str(const rctdl_generic_trace_elem *p_pkt, char *buffer, const int buffer_size)
{
    rctdl_err_t err = RCTDL_OK;
    if((buffer == NULL) || (buffer_size < 2))
        return RCTDL_ERR_INVALID_PARAM_VAL;
    std::string str;
    trcPrintElemToString<RctdlTraceElement,rctdl_generic_trace_elem>(p_pkt,str);
//    RctdlTraceElement::toString(p_pkt,str);
    if(str.size() > 0)
    {
        strncpy(buffer,str.c_str(),buffer_size -1);
        buffer[buffer_size-1] = 0;
    }
    return err;
}

RCTDL_C_API rctdl_err_t rctdl_dt_add_binfile_mem_acc(const dcd_tree_handle_t handle, const rctdl_vaddr_t address, const rctdl_mem_space_acc_t mem_space, const char *filepath)
{
    rctdl_err_t err = RCTDL_OK;

    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        DecodeTree *pDT = static_cast<DecodeTree *>(handle);
        if(!pDT->hasMemAccMapper())
            err = pDT->createMemAccMapper();

        if(err == RCTDL_OK)
        {
            TrcMemAccessorFile *pAcc = 0;
            std::string pathToFile = filepath;
            err = TrcMemAccessorFile::createFileAccessor(&pAcc,pathToFile,address);
            if(err == RCTDL_OK)
            {
                pAcc->setMemSpace(mem_space);
                pDT->addMemAccessorToMap(pAcc,0);
            }
        }
    }
    else
        err = RCTDL_ERR_INVALID_PARAM_VAL;
    return err;
}

RCTDL_C_API rctdl_err_t rctdl_dt_add_buffer_mem_acc(const dcd_tree_handle_t handle, const rctdl_vaddr_t address, const rctdl_mem_space_acc_t mem_space, const uint8_t *p_mem_buffer, const uint32_t mem_length)
{
    rctdl_err_t err = RCTDL_OK;

    if(handle != C_API_INVALID_TREE_HANDLE)
    {
        DecodeTree *pDT = static_cast<DecodeTree *>(handle);
        if(!pDT->hasMemAccMapper())
            err = pDT->createMemAccMapper();

        if(err == RCTDL_OK)
        {
            TrcMemAccBufPtr *pBuff = new (std::nothrow) TrcMemAccBufPtr(address,p_mem_buffer,mem_length);
            if(pBuff)
            {
                pBuff->setMemSpace(mem_space);
                pDT->addMemAccessorToMap(pBuff,0);
            }
            else 
                err = RCTDL_ERR_MEM;
        }
    }
    else
        err = RCTDL_ERR_INVALID_PARAM_VAL;
    return err;
}

/*******************************************************************************/
/* C API Helper objects                                                        */
/*******************************************************************************/

/****************** Generic trace element output callback function  ************/
GenTraceElemCBObj::GenTraceElemCBObj(FnTraceElemIn pCBFn) :
    m_c_api_cb_fn(pCBFn)
{
}

rctdl_datapath_resp_t GenTraceElemCBObj::TraceElemIn(const rctdl_trc_index_t index_sop,
                                              const uint8_t trc_chan_id,
                                              const RctdlTraceElement &elem)
{
    return m_c_api_cb_fn(index_sop, trc_chan_id, &elem);
}

/****************** Etmv4 packet processor output callback function  ************/
EtmV4ICBObj::EtmV4ICBObj(FnEtmv4IPacketDataIn pCBFn) :
    m_c_api_cb_fn(pCBFn)
{
}

rctdl_datapath_resp_t EtmV4ICBObj::PacketDataIn( const rctdl_datapath_op_t op,
                                                 const rctdl_trc_index_t index_sop,
                                                 const EtmV4ITrcPacket *p_packet_in)
{
    return m_c_api_cb_fn(op,index_sop,p_packet_in);
}

EtmV4IPktMonCBObj::EtmV4IPktMonCBObj(FnEtmv4IPktMonDataIn pCBFn) :
    m_c_api_cb_fn(pCBFn)
{
}
    
/****************** Etmv4 packet processor monitor callback function  ***********/
void EtmV4IPktMonCBObj::RawPacketDataMon( const rctdl_datapath_op_t op,
                                   const rctdl_trc_index_t index_sop,
                                   const EtmV4ITrcPacket *p_packet_in,
                                   const uint32_t size,
                                   const uint8_t *p_data)
{
    return m_c_api_cb_fn(op, index_sop, p_packet_in, size, p_data);
}

/* End of File rctdl_c_api.cpp */

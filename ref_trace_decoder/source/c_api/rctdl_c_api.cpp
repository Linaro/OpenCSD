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

/* pull in the C++ decode library */
#include "rctdl.h"
#include "c_api\rctdl_c_api.h"
#include "rctdl_c_api_obj.h"

/*******************************************************************************/
/* C API functions                                                             */
/*******************************************************************************/


RCTDL_C_API dcd_tree_handle_t rcdtl_create_dcd_tree(const rctdl_dcd_tree_src_t src_type, const uint32_t deformatterCfgFlags)
{
    dcd_tree_handle_t handle = 0;
    handle = (dcd_tree_handle_t)DecodeTree::CreateDecodeTree(src_type,deformatterCfgFlags); 
    return handle;
}

RCTDL_C_API void rcdtl_destroy_dcd_tree(const dcd_tree_handle_t handle)
{
    if(handle != 0)
    {
        GenTraceElemCBObj * pIf = (GenTraceElemCBObj *)(((DecodeTree *)handle)->getGenTraceElemOutI());
        if(pIf != 0)
            delete pIf;
        DecodeTree::DestroyDecodeTree((DecodeTree *)handle);
    }
}

RCTDL_C_API rctdl_datapath_resp_t rcdtl_dt_process_data(const dcd_tree_handle_t handle,
                                            const rctdl_datapath_op_t op,
                                            const rctdl_trc_index_t index,
                                            const uint32_t dataBlockSize,
                                            const uint8_t *pDataBlock,
                                            uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp =  RCTDL_RESP_FATAL_NOT_INIT;
    if(handle != 0)
        resp = ((DecodeTree *)handle)->TraceDataIn(op,index,dataBlockSize,pDataBlock,numBytesProcessed);
    return resp;
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


/*******************************************************************************/
/* C API Helper objects                                                        */
/*******************************************************************************/

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
/* End of File rctdl_c_api.cpp */

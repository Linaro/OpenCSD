/*
 * \file       trc_frame_deformatter.cpp
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
#include <cstring>

#include "trc_frame_deformatter.h"
#include "trc_frame_deformatter_impl.h"

/***************************************************************/
/* Implementation */
/***************************************************************/

#ifdef __GNUC__
// G++ doesn't like the ## pasting
#define DEFORMATTER_NAME "DFMT_CSFRAMES"
#else
// VC is fine
#define DEFORMATTER_NAME RCTDL_CMPNAME_PREFIX_FRAMEDEFORMATTER##"_CSFRAMES"
#endif

TraceFmtDcdImpl::TraceFmtDcdImpl() : TraceComponent(DEFORMATTER_NAME),
    m_cfgFlags(0),
    m_force_sync_idx(0),
    m_use_force_sync(false),
    m_alignment(16) // assume frame aligned data as default.
{
    resetStateParams();
}

TraceFmtDcdImpl::TraceFmtDcdImpl(int instNum) : TraceComponent(DEFORMATTER_NAME, instNum),
    m_cfgFlags(0),
    m_force_sync_idx(0),
    m_use_force_sync(false),
    m_alignment(16)
{
    resetStateParams();
}

TraceFmtDcdImpl::~TraceFmtDcdImpl()
{
}

rctdl_datapath_resp_t TraceFmtDcdImpl::TraceDataIn(
    const rctdl_datapath_op_t op, 
    const rctdl_trc_index_t index, 
    const uint32_t dataBlockSize, 
    const uint8_t *pDataBlock, 
    uint32_t *numBytesProcessed)
{
    rctdl_datapath_resp_t resp = RCTDL_RESP_FATAL_INVALID_OP;
    InitCollateDataPathResp();

    switch(op)
    {
    case RCTDL_OP_RESET: 
        resp = Reset();
        break;

    case RCTDL_OP_FLUSH:
        resp = Flush();
        break;

    case RCTDL_OP_EOT:
        // local 'flush' here?
        // pass on EOT to connected ID streams
        resp = executeNoneDataOpAllIDs(RCTDL_OP_EOT);
        break;

    case RCTDL_OP_DATA:
        if((dataBlockSize <= 0) || ( pDataBlock == 0) || (numBytesProcessed == 0))
            resp = RCTDL_RESP_FATAL_INVALID_PARAM;
        else
            resp = processTraceData(index,dataBlockSize, pDataBlock, numBytesProcessed);
        break;

    default:
        break;
    }

    return resp;
}

/* enable / disable ID streams - default as all enabled */
rctdl_err_t TraceFmtDcdImpl::OutputFilterIDs(std::vector<uint8_t> id_list, bool bEnable)
{
    rctdl_err_t err =  RCTDL_OK;
    std::vector<uint8_t>::iterator iter = id_list.begin();
    uint8_t id = 0;

    while((iter < id_list.end()) && (err == RCTDL_OK))
    {
        id = *iter;
        if(id > 128)
            err = RCTDL_ERR_INVALID_ID;
        else
            m_IDStreams[id].set_enabled(bEnable);
        iter++;
    }
    return err;
}

rctdl_err_t TraceFmtDcdImpl::OutputFilterAllIDs(bool bEnable)
{
    for(uint8_t id = 0; id < 128; id++)
    {
        m_IDStreams[id].set_enabled(bEnable);
    }
    return RCTDL_OK;
}

/* decode control */
rctdl_datapath_resp_t TraceFmtDcdImpl::Reset()
{
    resetStateParams();
    InitCollateDataPathResp();
    return executeNoneDataOpAllIDs(RCTDL_OP_RESET);
}

rctdl_datapath_resp_t TraceFmtDcdImpl::Flush()
{
    executeNoneDataOpAllIDs(RCTDL_OP_FLUSH);    // flush any upstream data.
    if(dataPathCont())
        outputFrame();  // try to flush any partial frame data remaining
    return highestDataPathResp();
}

rctdl_datapath_resp_t TraceFmtDcdImpl::executeNoneDataOpAllIDs(rctdl_datapath_op_t op)
{
    ITrcDataIn *pTrcComp = 0;
    for(uint8_t id = 0; id < 128; id++)
    {
        if(m_IDStreams[id].num_attached())
        {
            pTrcComp = m_IDStreams[id].first();
            while(pTrcComp)
            {
                CollateDataPathResp(pTrcComp->TraceDataIn(op,0,0,0,0));
                pTrcComp = m_IDStreams[id].next();
            }
        }
    }

    if( m_RawTraceFrame.num_attached())
    {
        if(m_RawTraceFrame.first())
            CollateDataPathResp(m_RawTraceFrame.first()->TraceRawFrameIn(op,0,RCTDL_FRM_NONE,0,0));
    }
    return highestDataPathResp();
}

void TraceFmtDcdImpl::CollateDataPathResp(const rctdl_datapath_resp_t resp)
{
    // simple most severe error across multiple IDs.
    if(resp > m_highestResp) m_highestResp = resp;
}

rctdl_datapath_resp_t TraceFmtDcdImpl::processTraceData( 
                                        const rctdl_trc_index_t index, 
                                        const uint32_t dataBlockSize, 
                                        const uint8_t *pDataBlock, 
                                        uint32_t *numBytesProcessed
                                        )
{
    try {

        if(!m_first_data)  // is this the initial data block?
        {
            m_trc_curr_idx = index;
        }
        else
        {
            if(m_trc_curr_idx != index) // none continuous trace data - throw an error.
                throw rctdlError(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_DFMTR_NOTCONTTRACE,index);
        }
        
        if(dataBlockSize % m_alignment) // must be correctly aligned data 
        {
            rctdlError err(RCTDL_ERR_SEV_ERROR, RCTDL_ERR_INVALID_PARAM_VAL);
            char msg_buffer[64];
            sprintf(msg_buffer,"Input block incorrect size, must be %d byte multiple", m_alignment);
            err.setMessage(msg_buffer);
            throw rctdlError(&err);
        }

        // record the incoming block for extraction routines to use.
        m_in_block_base = pDataBlock;
        m_in_block_size = dataBlockSize;
        m_in_block_processed = 0;

        // processing loop...
        if(checkForSync())
        {
            bool bProcessing = true;
            while(bProcessing) 
            {
                bProcessing = extractFrame();   // will stop on end of input data.
                if(bProcessing)
                    bProcessing = unpackFrame();
                if(bProcessing)
                    bProcessing = outputFrame(); // will stop on data path halt.
            }
        }
    }
    catch(const rctdlError &err) {
        LogError(err);
        CollateDataPathResp(RCDTL_REST_FATAL_INVALID_DATA);
    }
    catch(...) {
        LogError(rctdlError(RCTDL_ERR_SEV_ERROR, RCTDL_ERR_FAIL));
        CollateDataPathResp(RCTDL_RESP_FATAL_SYS_ERR);
    }

    if(!m_first_data)
        m_first_data = true;
    
    // update the outputs.
    *numBytesProcessed = m_in_block_processed;

    return highestDataPathResp();
}

rctdl_err_t  TraceFmtDcdImpl::DecodeConfigure(uint32_t flags)
{
    char *pszErrMsg = "";
    rctdl_err_t err = RCTDL_OK;

    if((flags & ~RCTDL_DFRMTR_VALID_MASK) != 0)
    {
        err = RCTDL_ERR_INVALID_PARAM_VAL;
        pszErrMsg = "Unknown Config Flags";
    }

    if((flags & RCTDL_DFRMTR_VALID_MASK) == 0)
    {
        err = RCTDL_ERR_INVALID_PARAM_VAL;
        pszErrMsg = "No Config Flags Set";
    }

    if((flags & (RCTDL_DFRMTR_HAS_FSYNCS | RCTDL_DFRMTR_HAS_HSYNCS)) &&
       (flags & RCTDL_DFRMTR_FRAME_MEM_ALIGN)
       )
    {
        err = RCTDL_ERR_INVALID_PARAM_VAL;
        pszErrMsg = "Invalid Config Flag Combination Set";
    }

    if(err != RCTDL_OK)
    {
        rctdlError errObj(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_INVALID_PARAM_VAL);
        errObj.setMessage(pszErrMsg);
        LogError(errObj);
    }
    else
    {
        m_cfgFlags = flags;
        m_alignment = 16;
        if(flags & RCTDL_DFRMTR_HAS_FSYNCS)
            m_alignment = 4;
        else if(flags & RCTDL_DFRMTR_HAS_HSYNCS)
            m_alignment = 2;
    }
    return err;
}

void TraceFmtDcdImpl::resetStateParams()
{
    // overall dynamic state - intra frame
    m_trc_curr_idx = RCTDL_BAD_TRC_INDEX;    /* source index of current trace data */
    m_frame_synced = false;
    m_first_data = false;    
    m_curr_src_ID = RCTDL_BAD_CS_SRC_ID;

    // current frame processing
    m_ex_frm_n_bytes = 0;
    m_trc_curr_idx_sof = RCTDL_BAD_TRC_INDEX;
}

bool TraceFmtDcdImpl::checkForSync()
{
    // we can sync on:-
    // 16 byte alignment - standard input buffers such as ETB
    // FSYNC packets in the stream
    // forced index programmed into the object.
    uint32_t unsynced_bytes = 0;

    if(!m_frame_synced)
    {
        if(m_use_force_sync)
        {
            // is the force sync point in this block?
            if((m_force_sync_idx >= m_trc_curr_idx) && (m_force_sync_idx < (m_trc_curr_idx + m_in_block_size)))
            {
                unsynced_bytes = m_force_sync_idx - m_trc_curr_idx;
                m_frame_synced = true;
            }
            else
            {
                unsynced_bytes = m_in_block_size;
            }
        }
        else if( m_cfgFlags & RCTDL_DFRMTR_HAS_FSYNCS)   // memory aligned data
        {
             unsynced_bytes = findfirstFSync();

        }
        else
        {
            // RCTDL_DFRMTR_FRAME_MEM_ALIGN - this has guaranteed 16 byte frame size and alignment.
            m_frame_synced = true;
        }

        if(unsynced_bytes)
        {
            outputUnsyncedBytes(unsynced_bytes);
            m_in_block_processed = unsynced_bytes;
            m_trc_curr_idx += unsynced_bytes;
        }
    }
    return m_frame_synced;
}

uint32_t TraceFmtDcdImpl::findfirstFSync()
{
    uint32_t unsynced = m_in_block_size;  // consider entire block as unsynced at present.
#if 0
    if(m_match_fsync.patternSize() == 0)
    {
        uint8_t pattern[] = { 0xFF, 0xFF, 0xFF, 0x7F };
        if(!m_match_fsync.setPattern(pattern,sizeof(pattern)))
        {
            rctdlError err(RCTDL_ERR_SEV_ERROR, RCTDL_ERR_NOT_INIT);
            err.setMessage("Failed to set pattern in FSYNC matcher.");
            throw rctdlError(&err);
        }
    }

    // find an FSYNC
    if(m_match_fsync.checkBuffer(m_in_block_base,m_in_block_size))
    {
        unsynced = m_match_fsync.pos(); // number of unsynced bytes is the position in the buffer.
        m_frame_synced = true;
    }
#endif
    return unsynced;
}

void TraceFmtDcdImpl::outputUnsyncedBytes(uint32_t num_bytes)
{
}

bool TraceFmtDcdImpl::extractFrame()
{
    bool cont_process = true;   // continue processing after extraction.
    uint32_t f_h_sync_bytes = 0;

    // memory aligned sources are always multiples of frames, aligned to start.
    if( m_cfgFlags & RCTDL_DFRMTR_FRAME_MEM_ALIGN)
    {
        if(m_in_block_processed == m_in_block_size)
        {
            m_ex_frm_n_bytes = 0;
            cont_process = false;   // end of input data.
        }
        else
        {
            // always a complete frame.
            m_ex_frm_n_bytes = RCTDL_DFRMTR_FRAME_SIZE;
            memcpy(m_ex_frm_data, m_in_block_base+m_in_block_processed,m_ex_frm_n_bytes);
            m_trc_curr_idx_sof = m_trc_curr_idx;

            // TBD  output raw data on raw frame channel.
        }
    }
    else
    {
        // extract data accounting for frame syncs and hsyncs if present.
        // we know we are aligned at this point - could be FSYNC or HSYNCs here.

        #define FSYNC_PATTERN 0x7FFFFFFF    // LE host pattern for FSYNC 
        #define HSYNC_PATTERN 0x7FFF        // LE host pattern for HSYNC

        // check what we a looking for
        bool hasFSyncs =  ((m_cfgFlags & RCTDL_DFRMTR_HAS_FSYNCS) == RCTDL_DFRMTR_HAS_FSYNCS);
        bool hasHSyncs =  ((m_cfgFlags & RCTDL_DFRMTR_HAS_HSYNCS) == RCTDL_DFRMTR_HAS_HSYNCS);

        const uint8_t *dataPtr = m_in_block_base+m_in_block_processed;
        const uint8_t *eodPtr = m_in_block_base+m_in_block_size;
        
        cont_process = (bool)(dataPtr < eodPtr);
        
        // can have FSYNCS at start of frame (in middle is an error).
        if(hasFSyncs && cont_process)
        {
            while((*((uint32_t *)(dataPtr)) == (uint32_t)FSYNC_PATTERN) && cont_process)
            {
                f_h_sync_bytes += 4;
                dataPtr += 4;
                cont_process = (bool)(dataPtr < eodPtr);
            }
        }

        // not an FSYNC
        while((m_ex_frm_n_bytes < RCTDL_DFRMTR_FRAME_SIZE) && cont_process)
        {
            // check for illegal out of sequence FSYNC
            if((m_ex_frm_n_bytes % 4) == 0)
            {
                if(*((uint32_t *)(dataPtr)) == (uint32_t)FSYNC_PATTERN) 
                {
                    // throw an illegal FSYNC error
                }
            }

            if(m_ex_frm_n_bytes == 0)
                m_trc_curr_idx_sof = m_trc_curr_idx;

            m_ex_frm_data[m_ex_frm_n_bytes] = dataPtr[0];
            m_ex_frm_data[m_ex_frm_n_bytes+1] = dataPtr[1];
            m_ex_frm_n_bytes+=2;

            // check pair is not HSYNC
            if(*((uint16_t *)(dataPtr)) == (uint16_t)HSYNC_PATTERN)
            {
                if(hasHSyncs)
                {
                    m_ex_frm_n_bytes-=2;
                    f_h_sync_bytes+=2;
                }
                else
                {
                    // throw illegal HSYNC error.
                }
            }

            dataPtr += 2;
            cont_process = (bool)(dataPtr < eodPtr);
        }

        // if we hit the end of data but still have a complete frame waiting, 
        // need to continue processing to allow it to be used.
        if(!cont_process && (m_ex_frm_n_bytes == RCTDL_DFRMTR_FRAME_SIZE))
            cont_process = true;
    }

    // update the processed count for the buffer
    m_in_block_processed += m_ex_frm_n_bytes + f_h_sync_bytes;

    // init processing for the extracted frame...    
    m_trc_curr_idx += m_ex_frm_n_bytes + f_h_sync_bytes;

    return cont_process;
}

bool TraceFmtDcdImpl::unpackFrame()
{
    // unpack cannot fail as never called on incomplete frame.
    uint8_t frameFlagBit = 0x1;
    uint8_t newSrcID = RCTDL_BAD_CS_SRC_ID;
    bool PrevID = false;

    // init output processing
    m_out_data_idx = 0;   
    m_out_processed = 0;

    // set up first out data packet...
    m_out_data[m_out_data_idx].id = m_curr_src_ID;
    m_out_data[m_out_data_idx].valid = 0;
    m_out_data[m_out_data_idx].index =  m_trc_curr_idx_sof;
    m_out_data[m_out_data_idx].used = 0;

    // work on byte pairs - bytes 0 - 13.
    for(int i = 0; i < 14; i+=2)
    {
        PrevID = false;

        // it's an ID + data
        if(m_ex_frm_data[i] & 0x1)
        {
            newSrcID = (m_ex_frm_data[i] >> 1) & 0x7f;
            if(newSrcID != m_curr_src_ID)   // ID change
            {
                PrevID = ((frameFlagBit & m_ex_frm_data[15]) != 0);

                if(PrevID)
                    // 2nd byte always data
                    m_out_data[m_out_data_idx].data[m_out_data[m_out_data_idx].valid++] = m_ex_frm_data[i+1];

                // change ID
                m_curr_src_ID = newSrcID;
                m_out_data_idx++;
                m_out_data[m_out_data_idx].id = m_curr_src_ID;
                m_out_data[m_out_data_idx].valid = 0;
                m_out_data[m_out_data_idx].used = 0;
                m_out_data[m_out_data_idx].index = m_trc_curr_idx_sof + i;
            }
            /// TBD - ID indexing in here.
        }
        else
        // it's just data
        {
            m_out_data[m_out_data_idx].data[m_out_data[m_out_data_idx].valid++] = m_ex_frm_data[i] | (frameFlagBit & m_ex_frm_data[15]) ? 0x1 : 0x0;             
        }

        // 2nd byte always data
        if(!PrevID) // output only if we didn't for an ID change + prev ID.
            m_out_data[m_out_data_idx].data[m_out_data[m_out_data_idx].valid++] = m_ex_frm_data[i+1];

        frameFlagBit <<= 1;
    }

    // unpack byte 14;

    // it's an ID
    if(m_ex_frm_data[14] & 0x1)
    {
        // no matter if change or not, no associated data in byte 15 anyway so just set.
        m_curr_src_ID = (m_ex_frm_data[14] >> 1) & 0x7f;
    }
    // it's data
    else
    {
        m_out_data[m_out_data_idx].data[m_out_data[m_out_data_idx].valid++] = m_ex_frm_data[14] | (frameFlagBit & m_ex_frm_data[15]) ? 0x1 : 0x0;             
    }
    m_ex_frm_n_bytes = 0;   // mark frame as empty;
    return true;
}

// output data to channels.
bool TraceFmtDcdImpl::outputFrame()
{
    bool cont_processing = true;
    ITrcDataIn *pDataIn = 0;
    uint32_t bytes_used;

    while((m_out_processed < (m_out_data_idx + 1)) && cont_processing)
    {
        // may have data prior to a valid ID appearing
        if(m_out_data[m_out_processed].id != RCTDL_BAD_CS_SRC_ID)
        {
            if((pDataIn = m_IDStreams[m_out_data[m_out_processed].id].first()) != 0)
            {
                CollateDataPathResp(pDataIn->TraceDataIn(RCTDL_OP_DATA,
                    m_out_data[m_out_processed].index,
                    m_out_data[m_out_processed].valid - m_out_data[m_out_processed].used,
                    m_out_data[m_out_processed].data + m_out_data[m_out_processed].used,
                    &bytes_used));
                
                if(!dataPathCont())
                {
                    cont_processing = false;
                    m_out_data[m_out_processed].used += bytes_used;
                }
                else
                     m_out_processed++; // we have sent this data;
            }
            else
                m_out_processed++; // skip past this data.
        }
        else
        {
            m_out_processed++; // skip past this data.
            /// TBD - output this on the RAW channel.
        }
    }
    return cont_processing;
}

/***************************************************************/
/* interface */
/***************************************************************/
TraceFormatterFrameDecoder::TraceFormatterFrameDecoder() : m_pDecoder(0)
{
    m_instNum = -1;
}

TraceFormatterFrameDecoder::TraceFormatterFrameDecoder(int instNum) : m_pDecoder(0)
{
    m_instNum = instNum;
}

TraceFormatterFrameDecoder::~TraceFormatterFrameDecoder()
{
    if(m_pDecoder) 
    {
        delete m_pDecoder;
        m_pDecoder = 0;
    }
}

    /* the data input interface from the reader / source */
rctdl_datapath_resp_t TraceFormatterFrameDecoder::TraceDataIn(  const rctdl_datapath_op_t op, 
                                                                const rctdl_trc_index_t index, 
                                                                const uint32_t dataBlockSize, 
                                                                const uint8_t *pDataBlock, 
                                                                uint32_t *numBytesProcessed)
{
    return (m_pDecoder == 0) ? RCTDL_RESP_FATAL_NOT_INIT : m_pDecoder->TraceDataIn(op,index,dataBlockSize,pDataBlock,numBytesProcessed);
}

/* attach a data processor to a stream ID output */
componentAttachPt<ITrcDataIn> *TraceFormatterFrameDecoder::getIDStreamAttachPt(uint8_t ID)
{
    componentAttachPt<ITrcDataIn> *pAttachPt = 0;
    if((ID < 128) && (m_pDecoder != 0))
        pAttachPt = &(m_pDecoder->m_IDStreams[ID]);
    return pAttachPt;
}

/* attach a data processor to the raw frame output */
componentAttachPt<ITrcRawFrameIn> *TraceFormatterFrameDecoder::getTrcRawFrameAttachPt()
{
    return (m_pDecoder != 0) ? &m_pDecoder->m_RawTraceFrame : 0;
}


componentAttachPt<ITrcSrcIndexCreator> *TraceFormatterFrameDecoder::getTrcSrcIndexAttachPt()
{
    return (m_pDecoder != 0) ? &m_pDecoder->m_SrcIndexer : 0;
}

componentAttachPt<ITraceErrorLog> *TraceFormatterFrameDecoder::getErrLogAttachPt()
{
    return (m_pDecoder != 0) ? m_pDecoder->getErrorLogAttachPt() : 0;
}

/* configuration - set operational mode for incoming stream (has FSYNCS etc) */
rctdl_err_t TraceFormatterFrameDecoder::Configure(uint32_t cfg_flags)
{
    if(!m_pDecoder) 
    {  
        if(m_instNum >= 0)
            m_pDecoder = new (std::nothrow) TraceFmtDcdImpl(m_instNum);
        else
            m_pDecoder = new (std::nothrow) TraceFmtDcdImpl();
        if(!m_pDecoder) return RCTDL_ERR_MEM;
    }
    m_pDecoder->m_cfgFlags = cfg_flags;
    return RCTDL_OK;
}

/* enable / disable ID streams - default as all enabled */
rctdl_err_t TraceFormatterFrameDecoder::OutputFilterIDs(std::vector<uint8_t> id_list, bool bEnable)
{
    return (m_pDecoder == 0) ? RCTDL_ERR_NOT_INIT : m_pDecoder->OutputFilterIDs(id_list,bEnable);
}

rctdl_err_t TraceFormatterFrameDecoder::OutputFilterAllIDs(bool bEnable)
{
    return (m_pDecoder == 0) ? RCTDL_ERR_NOT_INIT : m_pDecoder->OutputFilterAllIDs(bEnable);
}

/* decode control */
rctdl_datapath_resp_t TraceFormatterFrameDecoder::Reset()
{
    return (m_pDecoder == 0) ? RCTDL_RESP_FATAL_NOT_INIT : m_pDecoder->Reset();
}

rctdl_datapath_resp_t TraceFormatterFrameDecoder::Flush()
{
    return (m_pDecoder == 0) ? RCTDL_RESP_FATAL_NOT_INIT : m_pDecoder->Flush();
}


/* End of File trc_frame_deformatter.cpp */

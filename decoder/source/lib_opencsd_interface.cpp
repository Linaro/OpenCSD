/******************************************************************************
       Module: lib_opencsd_interface.cpp
     Engineer: Arjun Suresh
  Description: Implementation for OpenCSD Trace Decoder and related classes
  Date           Initials    Description
  30-Aug-2022    AS          Initial
******************************************************************************/
#include <limits.h>
#include "lib_opencsd_interface.h"
#include "opencsd/c_api/ocsd_c_api_types.h"

// Class for attaching packet monitor callback
template<class TrcPkt>
class PktMonCBObj : public IPktRawDataMon<TrcPkt>
{
public:
    PktMonCBObj(FnDefPktDataMon pCBFunc, const void* p_context)
    {
        m_c_api_cb_fn = pCBFunc;
        m_p_context = p_context;
    };

    virtual ~PktMonCBObj() {};

    virtual void RawPacketDataMon(const ocsd_datapath_op_t op,
        const ocsd_trc_index_t index_sop,
        const TrcPkt* p_packet_in,
        const uint32_t size,
        const uint8_t* p_data)
    {
        const void* c_pkt_struct = 0;
        if (op == OCSD_OP_DATA)
            c_pkt_struct = p_packet_in->c_pkt(); // always output the c struct packet
        m_c_api_cb_fn(m_p_context, op, index_sop, c_pkt_struct, size, p_data);
    };

private:
    FnDefPktDataMon m_c_api_cb_fn;
    const void* m_p_context;
};

/****************************************************************************
     Function: CreateOpenCSDInterface
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: CreateOpenCSDInterface* - Pointer to interface class
  Description: Constuctor to create interface class object
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
OpenCSDInterface* CreateOpenCSDInterface()
{
    return new OpenCSDInterface;
}

/****************************************************************************
     Function: OpenCSDInterface
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Constuctor to Initialize Trace Decoder Class
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
OpenCSDInterface::OpenCSDInterface()
    : mp_tree(NULL),
    mp_logger(NULL)
{
}

/****************************************************************************
     Function: InitLogger
     Engineer: Arjun Suresh
        Input: log_file_path - The file path to store the trace output
               split_files - Enable splitting of output files
               max_rows_in_file - Max rows in file if splitting is enabled
       Output: None
       return: TyTraceDecodeError
  Description: Initialize the trace output logger
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::InitLogger(const char *log_file_path, const bool split_files, const uint32_t max_rows_in_file)
{
    if (mp_logger)
    {
        mp_logger->CloseLogFile();
        delete mp_logger;
        mp_logger = NULL;
    }
    mp_logger = new TraceLogger(log_file_path, split_files, max_rows_in_file);
    if (!mp_logger)
    {
        return TRACE_DECODER_INIT_ERR;
    }
    mp_tree->setGenTraceElemOutI(mp_logger);
    mp_logger->OpenLogFile();
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: InitDecodeTree
     Engineer: Arjun Suresh
        Input: src_type - Coresight Frame Formatted / Raw Data
               formatter_cfg_flags - Decoder Tree Config Flags
       Output: None
       return: TyTraceDecodeError
  Description: Initialize the Decoder Tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::InitDecodeTree(const ocsd_dcd_tree_src_t src_type, const uint32_t formatter_cfg_flags)
{
    mp_tree = DecodeTree::CreateDecodeTree(src_type, formatter_cfg_flags);
    if (mp_tree == NULL)
    {
        return TRACE_DECODER_INIT_ERR;
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: CreateETMv4Decoder
     Engineer: Arjun Suresh
        Input: config - ETMv4 Decoder configuration settings
               create_flags - Config flags
       Output: None
       return: TyTraceDecodeError
  Description: Create the ETMv4 Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateETMv4Decoder(const ocsd_etmv4_cfg config, int32_t create_flags)
{
    ocsd_err_t ret = OCSD_OK;
    EtmV4Config config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_ETMV4I, create_flags, &config_obj);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_CFG_ERR;
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: CreateETMv3Decoder
     Engineer: Arjun Suresh
        Input: config - ETMv3 Decoder configuration settings
               create_flags - Config flags
       Output: None
       return: TyTraceDecodeError
  Description: Create the ETMv3 Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateETMv3Decoder(const ocsd_etmv3_cfg config, int32_t create_flags)
{
    ocsd_err_t ret = OCSD_OK;
    EtmV3Config config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_ETMV3, create_flags, &config_obj);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_CFG_ERR;
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: CreateSTMDecoder
     Engineer: Arjun Suresh
        Input: config - STM Decoder configuration settings
               create_flags - Config flags
       Output: None
       return: TyTraceDecodeError
  Description: Create the STM Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateSTMDecoder(const ocsd_stm_cfg config, int32_t create_flags)
{
    ocsd_err_t ret = OCSD_OK;
    STMConfig config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_STM, create_flags, &config_obj);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_CFG_ERR;
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: CreatePTMDecoder
     Engineer: Arjun Suresh
        Input: config - PTM Decoder configuration settings
               create_flags - Config flags
       Output: None
       return: TyTraceDecodeError
  Description: Create the PTM Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreatePTMDecoder(const ocsd_ptm_cfg config, int32_t create_flags)
{
    ocsd_err_t ret = OCSD_OK;
    PtmConfig config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_PTM, create_flags, &config_obj);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_CFG_ERR;
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: CreateMemAccMapper
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: TyTraceDecodeError
  Description: Function to create the memory access mapper
               This must be called before adding the memory regions or
               callback
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateMemAccMapper()
{
    ocsd_err_t ret = OCSD_OK;
    ret = mp_tree->createMemAccMapper();
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_MEM_ACC_MAP_CREATE_ERR;
    }

    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: AddMemoryAccessMapFromBin
     Engineer: Arjun Suresh
        Input: region - The memory region array within the bin file to check
                        for opcode
               mem_space - Specify if memory region can be accessed only under
                           certain security/exception levels. By default always
                           use OCSD_MEM_SPACE_ANY
               num_regions - number of memory regions in array
               path - path to bin file
       Output: None
       return: TyTraceDecodeError
  Description: Add memory access map to access opcodes from bin file
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::AddMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path)
{
    ocsd_err_t ret = mp_tree->addBinFileRegionMemAcc(region, num_regions, mem_space, path);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_MEM_ACC_MAP_ADD_ERR;
    }

    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: UpdateMemoryAccessMapFromBin
     Engineer: Arjun Suresh
        Input: region - The memory region array within the bin file to check
                        for opcode
               mem_space - Specify if memory region can be accessed only under
                           certain security/exception levels. By default always
                           use OCSD_MEM_SPACE_ANY
               num_regions - number of memory regions in array
               path - path to bin file
       Output: None
       return: TyTraceDecodeError
  Description: Update memory access map to access opcodes from bin file
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::UpdateMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path)
{
    ocsd_err_t ret = OCSD_OK;
    ret = mp_tree->updateBinFileRegionMemAcc(region, num_regions, mem_space, path);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_MEM_ACC_MAP_ADD_ERR;
    }

    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: AddMemoryAccessMapFromBuffer
     Engineer: Arjun Suresh
        Input: address - The address to map
               mem_space - Specify if memory region can be accessed only under
                           certain security/exception levels. By default always
                           use OCSD_MEM_SPACE_ANY
               p_mem_buffer - pointer to memory region buffer
               mem_length - memory region buffer length
       Output: None
       return: TyTraceDecodeError
  Description: Add memory access map to access opcodes from buffer
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::AddMemoryAccessMapFromBuffer(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t* p_mem_buffer, const uint32_t mem_length)
{
    ocsd_err_t ret = mp_tree->addBufferMemAcc(address, mem_space, p_mem_buffer, mem_length);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_MEM_ACC_MAP_ADD_ERR;
    }

    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: AddMemoryAccessCallback
     Engineer: Arjun Suresh
        Input: st_address - start address of memory region
               en_address - end address of memory region
               mem_space - Specify if memory region can be accessed only under
                           certain security/exception levels. By default always
                           use OCSD_MEM_SPACE_ANY
               p_cb_func - pointer to callback function
               p_context - The callback should copy the requested data to this
                           pointer
       Output: None
       return: TyTraceDecodeError
  Description: Decode the Trace file and save the decoded output
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::AddMemoryAccessCallback(const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void *p_context)
{
    ocsd_err_t ret = mp_tree->addCallbackMemAcc(st_address, en_address,
        mem_space, p_cb_func, p_context);
    if (ret != OCSD_OK)
    {
        return TRACE_DECODER_MEM_ACC_MAP_ADD_ERR;
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: SetPacketMonitorSink
     Engineer: Arjun Suresh
        Input: CSID - Source ID
               pDataInSink - Data sink pointer
               config_flags - Decoder config flags
       Output: None
       return: TyTraceDecodeError
  Description: Set the Packet Monitor Sink
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::SetPacketMonitorSink(const uint8_t CSID, ITrcTypedBase* pDataInSink, uint32_t config_flags)
{
    ocsd_err_t err = OCSD_OK;
    DecodeTreeElement* p_elem = mp_tree->getDecoderElement(CSID);
    if (p_elem == NULL)
    {
        return TRACE_DECODER_INVALID_HANDLE;
    }

    p_elem->getDecoderMngr()->attachPktMonitor(p_elem->getDecoderHandle(), pDataInSink);

    TraceFormatterFrameDecoder* p_deformatter = mp_tree->getFrameDeformatter();
    if (p_deformatter != NULL)
    {
        uint32_t deformmater_config_flags = p_deformatter->getConfigFlags();
        deformmater_config_flags |= config_flags;

        p_deformatter->Configure(deformmater_config_flags);

        RawFramePrinter* p_frame_printer = NULL;
        err = mp_tree->addRawFramePrinter(&p_frame_printer, deformmater_config_flags);
        if (err != OCSD_OK)
        {
            return TRACE_DECODER_ERR;
        }
    }
    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: SetPacketMonitorCallback
     Engineer: Arjun Suresh
        Input: CSID - Source ID
               p_fn_callback_data - Callback function
               p_context - Context
       Output: None
       return: TyTraceDecodeError
  Description: Set the Packet Monitor Callback
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::SetPacketMonitorCallback(const uint8_t CSID, void* p_fn_callback_data, const void* p_context)
{
    ocsd_err_t err = OCSD_OK;
    DecodeTreeElement* p_elem = mp_tree->getDecoderElement(CSID);
    if (p_elem == NULL)
    {
        return TRACE_DECODER_INVALID_HANDLE;
    }

    // Pointer to a sink callback object
    ITrcTypedBase* pDataInSink = NULL;
    FnDefPktDataMon pPktInFn = (FnDefPktDataMon)p_fn_callback_data;
    ocsd_trace_protocol_t protocol = p_elem->getProtocol();
    switch (protocol)
    {
    case OCSD_PROTOCOL_ETMV4I:
        pDataInSink = new (std::nothrow) PktMonCBObj<EtmV4ITrcPacket>(pPktInFn, p_context);
        break;

    case OCSD_PROTOCOL_ETMV3:
        pDataInSink = new (std::nothrow) PktMonCBObj<EtmV3TrcPacket>(pPktInFn, p_context);
        break;

    case OCSD_PROTOCOL_PTM:
        pDataInSink = new (std::nothrow) PktMonCBObj<PtmTrcPacket>(pPktInFn, p_context);
        break;

    case OCSD_PROTOCOL_STM:
        pDataInSink = new (std::nothrow) PktMonCBObj<StmTrcPacket>(pPktInFn, p_context);
        break;
    }

    p_elem->getDecoderMngr()->attachPktMonitor(p_elem->getDecoderHandle(), pDataInSink);

    TraceFormatterFrameDecoder* p_deformatter = mp_tree->getFrameDeformatter();
    if (p_deformatter != NULL)
    {
        uint32_t configFlags = p_deformatter->getConfigFlags();

        configFlags |= OCSD_DFRMTR_PACKED_RAW_OUT;
        configFlags |= OCSD_DFRMTR_UNPACKED_RAW_OUT;

        p_deformatter->Configure(configFlags);

        RawFramePrinter* p_frame_printer = NULL;
        err = mp_tree->addRawFramePrinter(&p_frame_printer, configFlags);
        if (err != OCSD_OK)
        {
            return TRACE_DECODER_ERR;
        }
    }
    return TRACE_DECODER_OK;
}


/****************************************************************************
     Function: SetEOT
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: TyTraceDecodeError
  Description: Marks end of trace
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::SetEOT()
{
    ocsd_datapath_resp_t err = mp_tree->TraceDataIn(OCSD_OP_EOT, 0, 0, 0, 0);
    if (mp_logger)
        mp_logger->CloseLogFile();
    return (OCSD_DATA_RESP_IS_FATAL(err)) ? TRACE_DECODER_DATA_PATH_FATAL_ERR : TRACE_DECODER_OK;
}

/****************************************************************************
     Function: ResetDecoder
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: TyTraceDecodeError
  Description: Resets decoder state
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::ResetDecoder()
{
    ocsd_datapath_resp_t err = mp_tree->TraceDataIn(OCSD_OP_RESET, 0, 0, 0, 0);
    if (mp_logger)
        mp_logger->CloseLogFile();
    return (OCSD_DATA_RESP_IS_FATAL(err)) ? TRACE_DECODER_DATA_PATH_FATAL_ERR : TRACE_DECODER_OK;
}

/****************************************************************************
     Function: DecodeTraceBuffer
     Engineer: Arjun Suresh
        Input: trace_buffer - buffer containing raw trace data
               size - size of trace data buffer
       Output: None
       return: TyTraceDecodeError
  Description: Function to decode trace data from buffer
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::DecodeTraceBuffer(uint8_t* trace_buffer, uint64_t size, uint64_t block_idx)
{
    if(trace_buffer == NULL)
    {
        TRACE_DECODER_CANNOT_OPEN_FILE;
    }
    ocsd_datapath_resp_t dataPathResp = OCSD_RESP_CONT;
    uint64_t nBuffRead = size;                           // get count of data loaded.
    uint32_t nBuffProcessed = 0;                        // amount processed in this buffer.
    uint32_t nUsedThisTime = 0;
    uint64_t trace_index = 0;

    // process the current buffer load until buffer done, or fatal error occurs
    while ((nBuffProcessed < nBuffRead) && !OCSD_DATA_RESP_IS_FATAL(dataPathResp))
    {
        if (OCSD_DATA_RESP_IS_CONT(dataPathResp))
        {
            dataPathResp = mp_tree->TraceDataIn(
                OCSD_OP_DATA,
                block_idx + trace_index,
                (uint32_t)(nBuffRead - nBuffProcessed),
                &(trace_buffer[0]) + nBuffProcessed,
                &nUsedThisTime);

            nBuffProcessed += nUsedThisTime;
            trace_index += nUsedThisTime;
            if (dataPathResp == OCSD_RESP_REACHED_STOP_IDX)
            {
                mp_tree->TraceDataIn(OCSD_OP_FLUSH, 0, 0, 0, 0);
                return TRACE_DECODER_READ_STOP_IDX_OK;
            }
        }
    }

    // fatal error - no futher processing
    if (OCSD_DATA_RESP_IS_FATAL(dataPathResp))
    {
        return TRACE_DECODER_DATA_PATH_FATAL_ERR;
    }

    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: DecodeTrace
     Engineer: Arjun Suresh
        Input: trace_in_file - Trace File captured from target
       Output: None
       return: TyTraceDecodeError
  Description: Decode the Trace file and save the decoded output
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::DecodeTrace(const char* trace_in_file)
{
    if (trace_in_file == NULL)
    {
        TRACE_DECODER_CANNOT_OPEN_FILE;
    }
    // need to push the data through the decode tree.
    std::ifstream in;
    in.open(trace_in_file, std::ifstream::in | std::ifstream::binary);
    if (in.is_open())
    {
        ocsd_datapath_resp_t dataPathResp = OCSD_RESP_CONT;
        static const int bufferSize = 1024;
        uint8_t trace_buffer[bufferSize];   // temporary buffer to load blocks of data from the file
        uint32_t trace_index = 0;           // index into the overall trace buffer (file).

        // process the file, a buffer load at a time
        while (!in.eof() && !OCSD_DATA_RESP_IS_FATAL(dataPathResp))
        {
            in.read((char *) &trace_buffer[0], bufferSize);   // load a block of data into the buffer

            std::streamsize nBuffRead = in.gcount();    // get count of data loaded.
            std::streamsize nBuffProcessed = 0;         // amount processed in this buffer.
            uint32_t nUsedThisTime = 0;

            // process the current buffer load until buffer done, or fatal error occurs
            while ((nBuffProcessed < nBuffRead) && !OCSD_DATA_RESP_IS_FATAL(dataPathResp))
            {
                if (OCSD_DATA_RESP_IS_CONT(dataPathResp))
                {
                    dataPathResp = mp_tree->TraceDataIn(
                        OCSD_OP_DATA,
                        trace_index,
                        (uint32_t) (nBuffRead - nBuffProcessed),
                        &(trace_buffer[0]) + nBuffProcessed,
                        &nUsedThisTime);

                    nBuffProcessed += nUsedThisTime;
                    trace_index += nUsedThisTime;
                }
            }
        }

        // fatal error - no futher processing
        if (OCSD_DATA_RESP_IS_FATAL(dataPathResp))
        {
            in.close();
            if (mp_logger)
                mp_logger->CloseLogFile();
            return TRACE_DECODER_DATA_PATH_FATAL_ERR;
        }
        else
        {
            // mark end of trace into the data path
            mp_tree->TraceDataIn(OCSD_OP_EOT, 0, 0, 0, 0);
        }

        // close the input file.
        in.close();
        if (mp_logger)
            mp_logger->CloseLogFile();
    }
    else
    {
        return TRACE_DECODER_CANNOT_OPEN_FILE;
    }

    return TRACE_DECODER_OK;
}

/****************************************************************************
     Function: GetFirstValidIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Returns first valid index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
uint64_t OpenCSDInterface::GetFirstValidIdx()
{
    return mp_logger->GetFirstValidIdx();
}

/****************************************************************************
     Function: SetTraceStopIdx
     Engineer: Arjun Suresh
        Input: uint64_t - trace stop index
       Output: None
       return: None
  Description: Sets the trace stop index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void OpenCSDInterface::SetTraceStopIdx(uint64_t index)
{
    return mp_logger->SetTraceStopIdx(index);
}

/****************************************************************************
     Function: ResetFirstValidIdxFlag
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Resets first valid index found flag
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void OpenCSDInterface::ResetFirstValidIdxFlag()
{
    mp_logger->ResetFirstValidIdxFlag();
}

/****************************************************************************
     Function: CloseLogFile
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Closes the current log file
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void OpenCSDInterface::CloseLogFile()
{
    // If FLUSH is called after data path error then decoder will crash
    //ocsd_datapath_resp_t err = mp_tree->TraceDataIn(OCSD_OP_FLUSH, 0, 0, NULL, NULL);
    if (mp_logger)
        mp_logger->CloseLogFile();
}

/****************************************************************************
     Function: ResetFirstValidIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Resets first valid index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void OpenCSDInterface::ResetFirstValidIdx()
{
    mp_logger->ResetFirstValidIdx();
}
/****************************************************************************
     Function: FirstValidIndexFound
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: bool - first valid index found flag
  Description: Check if first valid index found
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
bool OpenCSDInterface::FirstValidIndexFound()
{
    return mp_logger->FirstValidIndexFound();
}

/****************************************************************************
     Function: SetTraceStartIdx
     Engineer: Arjun Suresh
        Input: idx - trace start index
       Output: None
       return: None
  Description: Sets trace start index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void OpenCSDInterface::SetTraceStartIdx(uint64_t index)
{
    mp_logger->SetTraceStartIdx(index);
}

/****************************************************************************
     Function: GetLastValidIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: uint64_t
  Description: Returns last valid index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
uint64_t OpenCSDInterface::GetLastValidIdx()
{
    return mp_logger->GetLastValidIdx();
}

/****************************************************************************
     Function: GetLastPEContextIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: uint64_t
  Description: Returns last sync index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
uint64_t OpenCSDInterface::GetLastPEContextIdx()
{
    return mp_logger->GetLastPEContextIdx();
}

/****************************************************************************
     Function: DestroyDecodeTree
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Function to destroy the decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void OpenCSDInterface::DestroyDecodeTree()
{
    if (mp_tree != NULL)
    {
        DecodeTree::DestroyDecodeTree(mp_tree);
        mp_tree = NULL;
    }
    if (mp_logger != NULL)
    {
        delete mp_logger;
        mp_logger = NULL;
    }
}

/****************************************************************************
     Function: ~OpenCSDInterface
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Destructor for Trace Decoder
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
OpenCSDInterface::~OpenCSDInterface()
{
    DestroyDecodeTree();
}

/****************************************************************************
     Function: TraceLogger
     Engineer: Arjun Suresh
        Input: log_dir - Full path to file to output the decoded trace data
       Output: None
       return: None
  Description: Constructor to initialize the trace logger class
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TraceLogger::TraceLogger(const std::string log_file_path, const bool split_files, const uint32_t max_rows_in_file)
    : m_fp_decode_out(NULL),
    m_file_cnt(0),
    m_rows_in_file(0),
    m_split_files(split_files),
    m_max_rows_in_file(max_rows_in_file),
    m_log_file_path(log_file_path),
    m_curr_logfile_name(log_file_path),
    m_out_ex_level(true),
    m_cycle_cnt(0),
    m_last_timestamp(0),
    m_update_cycle_cnt(false),
    m_update_timestamp(false),
    m_first_valid_idx_found(false),
    m_first_valid_trace_idx(0),
    m_last_valid_trace_idx(ULLONG_MAX),
    m_trace_stop_idx(ULLONG_MAX),
    m_trace_start_idx(0),
    m_trace_stop_at_idx_flag(false),
    m_trace_start_from_idx_flag(false),
    m_last_pe_context_idx(0)
{

}


/****************************************************************************
     Function: FirstValidIndexFound
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: bool - first valid index found flag
  Description: Check if first valid index found
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
bool TraceLogger::FirstValidIndexFound()
{
    return m_first_valid_idx_found;
}

/****************************************************************************
     Function: ResetFirstValidIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Resets first valid index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void TraceLogger::ResetFirstValidIdx()
{
    m_first_valid_trace_idx = 0;
    m_first_valid_idx_found = false;
}

/****************************************************************************
     Function: SetTraceStopIdx
     Engineer: Arjun Suresh
        Input: uint64_t - trace stop index
       Output: None
       return: None
  Description: Sets the trace stop index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void TraceLogger::SetTraceStopIdx(uint64_t index)
{
    m_trace_stop_idx = index;
}

/****************************************************************************
     Function: ResetFirstValidIdxFlag
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Resets first valid index found flag
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void TraceLogger::ResetFirstValidIdxFlag()
{
    m_first_valid_idx_found = false;
}

/****************************************************************************
     Function: GetLastValidIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: uint64_t
  Description: Returns last valid index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
uint64_t TraceLogger::GetLastValidIdx()
{
    return m_last_valid_trace_idx;
}

/****************************************************************************
     Function: SetTraceStartIdx
     Engineer: Arjun Suresh
        Input: idx - trace start index
       Output: None
       return: None
  Description: Sets trace start index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void TraceLogger::SetTraceStartIdx(uint64_t idx)
{
    m_trace_start_idx = idx;
}

/****************************************************************************
     Function: GetFirstValidIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Returns first valid index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
uint64_t TraceLogger::GetFirstValidIdx()
{
    return m_first_valid_trace_idx;
}

/****************************************************************************
     Function: GetLastPEContextIdx
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: uint64_t
  Description: Returns last sync index
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
uint64_t TraceLogger::GetLastPEContextIdx()
{
    return m_last_pe_context_idx;
}

/****************************************************************************
     Function: OpenLogFile
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Open the log file to output decoded trace data
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void TraceLogger::OpenLogFile()
{
    //m_curr_logfile_name = m_log_file_path + (m_split_files ? std::to_string(m_file_cnt) : "");
#ifdef __linux__
    m_fp_decode_out = fopen(m_curr_logfile_name.c_str(), "a");
#else
    fopen_s(&m_fp_decode_out, m_curr_logfile_name.c_str(), "a");
#endif
}

/****************************************************************************
     Function: CloseLogFile
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Close the decoded trace log file
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
void TraceLogger::CloseLogFile()
{
    if (m_fp_decode_out)
    {
        fclose(m_fp_decode_out);
        m_fp_decode_out = NULL;
    }
}

/****************************************************************************
     Function: ~TraceLogger
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Destructor for trace logger class
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TraceLogger::~TraceLogger()
{
    CloseLogFile();
}

/****************************************************************************
     Function: ~TraceLogger
     Engineer: Arjun Suresh
        Input: index_sop - byte index in the decoded trace
               trc_chan_id - TID of the current trace row
               elem - contains the decoded trace info
       Output: None
       return: ocsd_datapath_resp_t - Used to halt/continue the decoder processing
  Description: Overriden function to implement custom data formatting
               This function is called by the opencsd library after decoding a chunk
               of data
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
ocsd_datapath_resp_t TraceLogger::TraceElemIn(const ocsd_trc_index_t index_sop,
    const uint8_t trc_chan_id,
    const OcsdTraceElement &elem)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;
    if(elem.elem_type != OCSD_GEN_TRC_ELEM_NO_SYNC)
        m_last_valid_trace_idx = index_sop;
    if (elem.elem_type == OCSD_GEN_TRC_ELEM_PE_CONTEXT)
    {
        m_last_pe_context_idx = index_sop;
    }
    if (m_first_valid_idx_found == false && elem.elem_type != OCSD_GEN_TRC_ELEM_NO_SYNC)
    {
        m_first_valid_trace_idx = index_sop;
        m_first_valid_idx_found = true;
    }
    if (index_sop < m_trace_start_idx)
    {
        return OCSD_RESP_CONT;
    }
    if (index_sop > m_trace_stop_idx)
    {
        if (m_update_timestamp == true)
        {
            m_update_timestamp = false;
            fprintf(m_fp_decode_out, "%s%llu\n", "TS:", m_last_timestamp);
        }
        return OCSD_RESP_REACHED_STOP_IDX;
    }
    if (elem.elem_type != OCSD_GEN_TRC_ELEM_TIMESTAMP && m_update_timestamp == true)
    {
        m_update_timestamp = false;
        fprintf(m_fp_decode_out, "%s%llu\n", "TS:", m_last_timestamp);
    }

    switch (elem.elem_type)
    {
    case OCSD_GEN_TRC_ELEM_PE_CONTEXT:
    {
        if (elem.context.ctxt_id_valid)
        {
            fprintf(m_fp_decode_out, "%s%u\n", "PECC:CID=", elem.context.context_id);
            m_rows_in_file++;
        }
    }
    break;
    case OCSD_GEN_TRC_ELEM_ADDR_NACC:
    {
        fprintf(m_fp_decode_out, "%u,%u,%u,", 0, ((trc_chan_id & 0x0F) >> 1), (elem.context.ctxt_id_valid ? elem.context.context_id : 0));
        if (m_update_cycle_cnt)
        {
            fprintf(m_fp_decode_out, "%u,", m_cycle_cnt);
            m_update_cycle_cnt = false;
        }
        else
        {
            fprintf(m_fp_decode_out, "%u,", 0);
        }
        fprintf(m_fp_decode_out, "%llu,%llx,%u,%u,0,%llx:MNA\n", m_last_timestamp, elem.st_addr, elem.num_instr_range, elem.last_instr_sz, elem.st_addr);
        m_rows_in_file++;
        m_cycle_cnt = 0;
    }
    break;
    case OCSD_GEN_TRC_ELEM_INSTR_RANGE:
    {
        // Check if we have stored the entire instruction sequence in elem.traced_ins.ptr_addresses array
        // If so, step through the array, else step through each address from start address in steps of last
        // instruction size
        ocsd_vaddr_t start_idx = elem.traced_ins.ptr_addresses ? 0 : elem.st_addr;
        ocsd_vaddr_t end_idx = elem.traced_ins.ptr_addresses ? elem.num_instr_range : elem.en_addr;
        ocsd_vaddr_t step = elem.traced_ins.ptr_addresses ? 1 : elem.last_instr_sz;
        for (ocsd_vaddr_t i = start_idx; i < end_idx; i+=step)
        {
            fprintf(m_fp_decode_out, "%u,%u,%u,", 0, ((trc_chan_id & 0x0F) >> 1), (elem.context.ctxt_id_valid ? elem.context.context_id : 0));
            if (m_update_cycle_cnt)
            {
                fprintf(m_fp_decode_out, "%u,", 0);
                m_update_cycle_cnt = false;
            }
            else if (elem.has_cc && i == start_idx)
            {
                fprintf(m_fp_decode_out, "%u,", elem.cycle_count);
            }
            else
            {
                fprintf(m_fp_decode_out, "%u,", 0);
            }
            fprintf(m_fp_decode_out, "%llu,%llx,%llx,%u,%u,", 0, elem.st_addr, elem.en_addr, elem.num_instr_range, elem.last_instr_sz);
            if ((elem.context.exception_level > ocsd_EL_unknown) && (elem.context.el_valid) && m_out_ex_level)
            {
                fprintf(m_fp_decode_out, "%s%d", "EL", (int) (elem.context.exception_level));
                switch (elem.context.security_level)
                {
                case ocsd_sec_secure: fprintf(m_fp_decode_out, "S,"); break;
                case ocsd_sec_nonsecure: fprintf(m_fp_decode_out, "N,"); break;
                case ocsd_sec_root: fprintf(m_fp_decode_out, "Root,"); break;
                case ocsd_sec_realm: fprintf(m_fp_decode_out, "Realm,"); break;
                }
            }
            else
            {
                fprintf(m_fp_decode_out, "%d,", 0);
            }
            fprintf(m_fp_decode_out, "%llx\n", elem.traced_ins.ptr_addresses ? elem.traced_ins.ptr_addresses[i] : i);
            m_rows_in_file++;
        }
        m_cycle_cnt = 0;
    }
    break;
    case OCSD_GEN_TRC_ELEM_EXCEPTION:
    {
        fprintf(m_fp_decode_out, "%s%u\n", "EX:", elem.exception_number);
        m_rows_in_file++;
    }
    break;
    case OCSD_GEN_TRC_ELEM_CYCLE_COUNT:
    {
        if (elem.has_cc)
        {
            fprintf(m_fp_decode_out, "CC = %u\n", elem.cycle_count);
            m_cycle_cnt = elem.cycle_count;
            m_update_cycle_cnt = true;
        }
    }
    break;
    case OCSD_GEN_TRC_ELEM_TIMESTAMP:
    {
        m_last_timestamp = elem.timestamp;
        m_update_timestamp = true;
        m_rows_in_file++;
    }
    break;
    }

    if (m_rows_in_file >= m_max_rows_in_file && m_split_files)
    {
        CloseLogFile();
        m_rows_in_file = 0;
        m_file_cnt++;
        OpenLogFile();
    }
    return resp;
}

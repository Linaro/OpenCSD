/******************************************************************************
       Module: lib_opencsd_interface.cpp
     Engineer: Arjun Suresh
  Description: Implementation for OpenCSD Trace Decoder and related classes
  Date           Initials    Description
  30-Aug-2022    AS          Initial
******************************************************************************/
#include "lib_opencsd_interface.h"

// Class responsible for formatting the decoding trace data
// This class is derived from ITrcGenElemIn defined in opencsd
class TraceLogger : public ITrcGenElemIn
{
protected:
private:
    FILE *m_fp_decode_out;
    const std::string m_log_file_path;
    std::string m_curr_logfile_name;
    uint32_t m_file_cnt;
    uint32_t m_rows_in_file;
    uint32_t m_cycle_cnt;
    uint64_t m_last_timestamp;
    const bool m_split_files;
    const uint32_t m_max_rows_in_file;
    bool m_update_cycle_cnt;
    bool m_out_ex_level;
public:
    // Constructor
    TraceLogger(const std::string log_file_path, const bool split_files = false, const uint32_t max_rows_in_file = DEAFAULT_MAX_TRACE_FILE_ROW_CNT);
    // Destructor
    ~TraceLogger();
    // Overriden function to implement custom formatting
    // This callback is called by the OpenCSD library after decoding a chunk of data
    ocsd_datapath_resp_t TraceElemIn(const ocsd_trc_index_t index_sop, const uint8_t trc_chan_id, const OcsdTraceElement &elem);
    // Open log file for logging decoded trace output
    void OpenLogFile();
    // Close trace decoded ouput log file
    void CloseLogFile();
};

// Singleton Class that provides the trace decoding functionality
// This class will use the OpenCSD decoder library
class OpenCSDInterface
{
protected:
private:
    DecodeTree *mp_tree;
    TraceLogger *mp_logger;
    // Private Constructor
    OpenCSDInterface();
    // Private Copy Constructor
    OpenCSDInterface(OpenCSDInterface &) :mp_tree(NULL) {}
public:
    // Get Instance function to be used to return the static object
    static OpenCSDInterface &GetInstance();
    // Function to initialize the deocder tree
    TyTraceDecodeError InitDecodeTree(const ocsd_dcd_tree_src_t src_type = OCSD_TRC_SRC_FRAME_FORMATTED,
        const uint32_t formatter_cfg_flags = OCSD_DFRMTR_FRAME_MEM_ALIGN);
    // Function to initialize the trace output logger
    TyTraceDecodeError InitLogger(const char *log_file_path, const bool split_files = false, const uint32_t max_rows_in_file = DEAFAULT_MAX_TRACE_FILE_ROW_CNT);
    // Function to create ETMv4 Decoder
    TyTraceDecodeError CreateETMv4Decoder(const ocsd_etmv4_cfg config);
    // Function to create ETMv3 Decoder
    TyTraceDecodeError CreateETMv3Decoder(const ocsd_etmv3_cfg config);
    // Function to create STM Decoder
    TyTraceDecodeError CreateSTMDecoder(const ocsd_stm_cfg config);
    // Function to create PTM Decoder
    TyTraceDecodeError CreatePTMDecoder(const ocsd_ptm_cfg config);
    // Function to create memory access mapper
    TyTraceDecodeError CreateMemAccMapper();
    // Function to add memory access map from bin file
    TyTraceDecodeError AddMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path);
    // Function to update memory access map from bin file
    TyTraceDecodeError UpdateMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path);
    // Function to add memory access map from buffer file
    TyTraceDecodeError AddMemoryAccessMapFromBuffer(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t* p_mem_buffer, const uint32_t mem_length);
    // Function to add memory access callback
    TyTraceDecodeError AddMemoryAccessCallback(const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void *p_context);
    // Function to decode the trace file
    TyTraceDecodeError DecodeTrace(const std::string &trace_in_file);
    // Function to destroy the decoder tree
    void DestroyDecodeTree();
    // Destructor
    ~OpenCSDInterface();
};

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
     Function: OpenCSDInterface
     Engineer: Arjun Suresh
        Input: None
       Output: None
       return: None
  Description: Function to return the static reference to the class object
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
OpenCSDInterface &OpenCSDInterface::GetInstance()
{
    static OpenCSDInterface s_opencsd_trace_decoder_obj;
    return s_opencsd_trace_decoder_obj;
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
    mp_logger = new TraceLogger(log_file_path, split_files, max_rows_in_file);
    if (!mp_logger)
    {
        return TRACE_DECODER_INIT_ERR;
    }
    mp_tree->setGenTraceElemOutI(mp_logger);
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
       Output: None
       return: TyTraceDecodeError
  Description: Create the ETMv4 Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateETMv4Decoder(const ocsd_etmv4_cfg config)
{
    ocsd_err_t ret = OCSD_OK;
    EtmV4Config config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_ETMV4I, OCSD_CREATE_FLG_FULL_DECODER, &config_obj);
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
       Output: None
       return: TyTraceDecodeError
  Description: Create the ETMv3 Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateETMv3Decoder(const ocsd_etmv3_cfg config)
{
    ocsd_err_t ret = OCSD_OK;
    EtmV3Config config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_ETMV3, OCSD_CREATE_FLG_FULL_DECODER, &config_obj);
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
       Output: None
       return: TyTraceDecodeError
  Description: Create the STM Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreateSTMDecoder(const ocsd_stm_cfg config)
{
    ocsd_err_t ret = OCSD_OK;
    STMConfig config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_STM, OCSD_CREATE_FLG_FULL_DECODER, &config_obj);
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
       Output: None
       return: TyTraceDecodeError
  Description: Create the PTM Decoder and attach it to decode tree
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::CreatePTMDecoder(const ocsd_ptm_cfg config)
{
    ocsd_err_t ret = OCSD_OK;
    PtmConfig config_obj(&config);
    ret = mp_tree->createDecoder(OCSD_BUILTIN_DCD_PTM, OCSD_CREATE_FLG_FULL_DECODER, &config_obj);
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
     Function: DecodeTrace
     Engineer: Arjun Suresh
        Input: trace_in_file - Trace File captured from target
       Output: None
       return: TyTraceDecodeError
  Description: Decode the Trace file and save the decoded output
  Date         Initials    Description
30-Aug-2022    AS          Initial
****************************************************************************/
TyTraceDecodeError OpenCSDInterface::DecodeTrace(const std::string &trace_in_file)
{
    // need to push the data through the decode tree.
    std::ifstream in;
    in.open(trace_in_file, std::ifstream::in | std::ifstream::binary);
    if (in.is_open())
    {
        mp_logger->OpenLogFile();
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
        mp_logger->CloseLogFile();
    }
    else
    {
        return TRACE_DECODER_CANNOT_OPEN_FILE;
    }

    return TRACE_DECODER_OK;
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
    m_update_cycle_cnt(false)
{
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
        fprintf(m_fp_decode_out, "%u,%u,%u,", index_sop, ((trc_chan_id & 0x0F) >> 1), (elem.context.ctxt_id_valid ? elem.context.context_id : 0));
        if (m_update_cycle_cnt)
        {
            fprintf(m_fp_decode_out, "%u,", m_cycle_cnt);
            m_update_cycle_cnt = false;
        }
        else
        {
            fprintf(m_fp_decode_out, "%u,", 0);
        }
        fprintf(m_fp_decode_out, "%llu,%x,%u,%u,0,%x:MNA\n", m_last_timestamp, elem.st_addr, elem.num_instr_range, elem.last_instr_sz, elem.st_addr);
        m_rows_in_file++;
        m_cycle_cnt = 0;
    }
    break;
    case OCSD_GEN_TRC_ELEM_INSTR_RANGE:
    {
        // Check if we have stored the entire instruction sequence in elem.traced_ins.ptr_addresses array
        // If so, step through the array, else step through each address from start address in steps of last
        // instruction size
        uint32_t start_idx = elem.traced_ins.ptr_addresses ? 0 : elem.st_addr;
        uint32_t end_idx = elem.traced_ins.ptr_addresses ? elem.num_instr_range : elem.en_addr;
        uint32_t step = elem.traced_ins.ptr_addresses ? 1 : elem.last_instr_sz;
        for (uint64_t i = start_idx; i < end_idx; i+=step)
        {
            fprintf(m_fp_decode_out, "%u,%u,%u,", index_sop, ((trc_chan_id & 0x0F) >> 1), (elem.context.ctxt_id_valid ? elem.context.context_id : 0));
            if (m_update_cycle_cnt)
            {
                fprintf(m_fp_decode_out, "%u,", m_cycle_cnt);
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
            fprintf(m_fp_decode_out, "%llu,%x,%x,%u,%u,", m_last_timestamp, elem.st_addr, elem.en_addr, elem.num_instr_range, elem.last_instr_sz);
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
            fprintf(m_fp_decode_out, "%x\n", elem.traced_ins.ptr_addresses ? elem.traced_ins.ptr_addresses[i] : i);
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
            m_cycle_cnt = elem.cycle_count;
            m_update_cycle_cnt = true;
        }
    }
    break;
    case OCSD_GEN_TRC_ELEM_TIMESTAMP:
    {
        m_last_timestamp = elem.timestamp;
        fprintf(m_fp_decode_out, "%s%llu\n", "TS:", m_last_timestamp);
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

// Exported Function to initialize the trace output logger
TyTraceDecodeError InitDecodeTree(const ocsd_dcd_tree_src_t src_type, const uint32_t formatter_cfg_flags)
{
    return OpenCSDInterface::GetInstance().InitDecodeTree(src_type, formatter_cfg_flags);
}

// Exported Function to initialize the trace output logger
TyTraceDecodeError InitLogger(const char *log_file_path, const bool split_files, const uint32_t max_rows_in_file)
{
    return OpenCSDInterface::GetInstance().InitLogger(log_file_path, split_files, max_rows_in_file);
}

// Exported Function to create ETMv4 Decoder
TyTraceDecodeError CreateETMv4Decoder(const ocsd_etmv4_cfg config)
{
    return OpenCSDInterface::GetInstance().CreateETMv4Decoder(config);
}

// Exported Function to create ETMv3 Decoder
TyTraceDecodeError CreateETMv3Decoder(const ocsd_etmv3_cfg config)
{
    return OpenCSDInterface::GetInstance().CreateETMv3Decoder(config);
}

// Exported Function to create STM Decoder
TyTraceDecodeError CreateSTMDecoder(const ocsd_stm_cfg config)
{
    return OpenCSDInterface::GetInstance().CreateSTMDecoder(config);
}

// Exported Function to create PTM Decoder
TyTraceDecodeError CreatePTMDecoder(const ocsd_ptm_cfg config)
{
    return OpenCSDInterface::GetInstance().CreatePTMDecoder(config);
}

// Exported Function to create Mem Access Mapper
TyTraceDecodeError CreateMemAccMapper()
{
    return OpenCSDInterface::GetInstance().CreateMemAccMapper();
}

// Exported Function to add memory access map from bin file
TyTraceDecodeError AddMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path)
{
    return OpenCSDInterface::GetInstance().AddMemoryAccessMapFromBin(region, mem_space, num_regions, path);
}

// Exported Function to update memory access map from bin file
TyTraceDecodeError UpdateMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path)
{
    return OpenCSDInterface::GetInstance().UpdateMemoryAccessMapFromBin(region, mem_space, num_regions, path);
}

// Exported Function to add memory access map from buffer file
TyTraceDecodeError AddMemoryAccessMapFromBuffer(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t* p_mem_buffer, const uint32_t mem_length)
{
    return OpenCSDInterface::GetInstance().AddMemoryAccessMapFromBuffer(address, mem_space, p_mem_buffer, mem_length);
}

// Exported Function to add memory access callback
TyTraceDecodeError AddMemoryAccessCallback(const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void *p_context)
{
    return OpenCSDInterface::GetInstance().AddMemoryAccessCallback(st_address, en_address, mem_space, p_cb_func, p_context);
}

// Exported Function to decode the trace file
TyTraceDecodeError DecodeOpenCSDTrace(const std::string trace_in_file)
{
    return OpenCSDInterface::GetInstance().DecodeTrace(trace_in_file);
}

// Exported Function to destroy the trace decoder tree
void DestroyDecodeTree()
{
    return OpenCSDInterface::GetInstance().DestroyDecodeTree();
}

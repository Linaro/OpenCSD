#pragma once
/******************************************************************************
       Module: lib_opencsd_interface.h
     Engineer: Arjun Suresh
  Description: Header for OpenCSD Trace Decoder and related classes
  Date           Initials    Description
  30-Aug-2022    AS          Initial
******************************************************************************/
#include "opencsd.h"
#include <iostream>
#include <fstream>

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the OPENCSDINTERFACE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// OPENCSDINTERFACE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef OPENCSDINTERFACE_EXPORTS
#ifdef __linux__ 
#define OPENCSDINTERFACE_API __attribute__ ((visibility ("default")))
#else
#define OPENCSDINTERFACE_API __declspec(dllexport)
#endif
#else
#ifdef __linux__
#define OPENCSDINTERFACE_API __attribute__ ((visibility ("default")))  
#else
#define OPENCSDINTERFACE_API __declspec(dllimport)
#endif
#endif

// Max no of row per decoded output file
#define DEAFAULT_MAX_TRACE_FILE_ROW_CNT 10000

// Trace Decoder Error Types
typedef enum
{
    TRACE_DECODER_OK,
    TRACE_DECODER_READ_STOP_IDX_OK,
    TRACE_DECODER_INIT_ERR,
    TRACE_DECODER_CFG_ERR,
    TRACE_DECODER_MEM_ACC_MAP_CREATE_ERR,
    TRACE_DECODER_MEM_ACC_MAP_ADD_ERR,
    TRACE_DECODER_DATA_PATH_FATAL_ERR,
    TRACE_DECODER_CANNOT_OPEN_FILE,
    TRACE_DECODER_INVALID_HANDLE,
    TRACE_DECODER_ERR
} TyTraceDecodeError;

// Class responsible for formatting the decoding trace data
// This class is derived from ITrcGenElemIn defined in opencsd
class TraceLogger : public ITrcGenElemIn
{
protected:
private:
    FILE* m_fp_decode_out;
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
    bool m_first_valid_idx_found;
    uint64_t m_first_valid_trace_idx;
    uint64_t m_last_valid_trace_idx;
    uint64_t m_last_pe_context_idx;
    bool m_trace_stop_at_idx_flag;
    bool m_trace_start_from_idx_flag;
    uint64_t m_trace_start_idx;
    uint64_t m_trace_stop_idx;
    bool m_update_timestamp;
public:
    // Constructor
    TraceLogger(const std::string log_file_path, const bool split_files = false, const uint32_t max_rows_in_file = DEAFAULT_MAX_TRACE_FILE_ROW_CNT);
    // Destructor
    ~TraceLogger();
    // Overriden function to implement custom formatting
    // This callback is called by the OpenCSD library after decoding a chunk of data
    ocsd_datapath_resp_t TraceElemIn(const ocsd_trc_index_t index_sop, const uint8_t trc_chan_id, const OcsdTraceElement& elem);
    // Open log file for logging decoded trace output
    void OpenLogFile();
    // Close trace decoded ouput log file
    void CloseLogFile();
    // Stop decoding at this index and return
    void SetTraceStopIdx(uint64_t index);
    // Ignore packets till this index
    void SetTraceStartIdx(uint64_t index);
    // Returns first valid trace index from trace decoding start
    uint64_t GetFirstValidIdx();
    // Returns last valid trace index
    uint64_t GetLastValidIdx();
    // Reset the first valid trace index
    void ResetFirstValidIdx();
    // Reset first valid index found flag
    void ResetFirstValidIdxFlag();
    // Get the last sync index
    uint64_t GetLastPEContextIdx();
    // Check if first valid index is found
    bool FirstValidIndexFound();
};

// Class that provides the trace decoding functionality
// This class will use the OpenCSD decoder library
class OpenCSDInterface
{
protected:
public:
    DecodeTree* mp_tree;
    TraceLogger* mp_logger;
public:
    OpenCSDInterface();
    // Get Instance function to be used to return the static object
    //static OpenCSDInterface& GetInstance();
    // Function to initialize the deocder tree
    virtual TyTraceDecodeError InitDecodeTree(const ocsd_dcd_tree_src_t src_type = OCSD_TRC_SRC_FRAME_FORMATTED,
        const uint32_t formatter_cfg_flags = OCSD_DFRMTR_FRAME_MEM_ALIGN);
    // Function to initialize the trace output logger
    virtual TyTraceDecodeError InitLogger(const char* log_file_path, const bool split_files = false, const uint32_t max_rows_in_file = DEAFAULT_MAX_TRACE_FILE_ROW_CNT);
    // Function to create ETMv4 Decoder
    virtual TyTraceDecodeError CreateETMv4Decoder(const ocsd_etmv4_cfg config, int32_t create_flags = OCSD_CREATE_FLG_FULL_DECODER);
    // Function to create ETMv3 Decoder
    virtual TyTraceDecodeError CreateETMv3Decoder(const ocsd_etmv3_cfg config, int32_t create_flags = OCSD_CREATE_FLG_FULL_DECODER);
    // Function to create STM Decoder
    virtual TyTraceDecodeError CreateSTMDecoder(const ocsd_stm_cfg config, int32_t create_flags = OCSD_CREATE_FLG_FULL_DECODER);
    // Function to create PTM Decoder
    virtual TyTraceDecodeError CreatePTMDecoder(const ocsd_ptm_cfg config, int32_t create_flags = OCSD_CREATE_FLG_FULL_DECODER);
    // Function to create memory access mapper
    virtual TyTraceDecodeError CreateMemAccMapper();
    // Function to add memory access map from bin file
    virtual TyTraceDecodeError AddMemoryAccessMapFromBin(const ocsd_file_mem_region_t* region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char* path);
    // Function to update memory access map from bin file
    virtual TyTraceDecodeError UpdateMemoryAccessMapFromBin(const ocsd_file_mem_region_t* region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char* path);
    // Function to add memory access map from buffer file
    virtual TyTraceDecodeError AddMemoryAccessMapFromBuffer(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t* p_mem_buffer, const uint32_t mem_length);
    // Function to add memory access callback
    virtual TyTraceDecodeError AddMemoryAccessCallback(const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void* p_context);
    // Function to decode the trace file
    virtual TyTraceDecodeError DecodeTrace(const char* trace_in_file);
    // Function to decode trace buffer
    virtual TyTraceDecodeError DecodeTraceBuffer(uint8_t* buffer, uint64_t size, uint64_t block_idx);
    // Function to get the created decode tree object
    virtual TyTraceDecodeError SetPacketMonitorCallback(const uint8_t CSID, void* p_fn_callback_data, const void* p_context);
    // Set the packet monitor sink
    virtual TyTraceDecodeError SetPacketMonitorSink(const uint8_t CSID, ITrcTypedBase* pDataInSink, uint32_t config_flags);
    // Mark the end of trace
    virtual TyTraceDecodeError SetEOT();
    // Reset the decode state
    virtual TyTraceDecodeError ResetDecoder();
    // Get first valid index
    virtual uint64_t GetFirstValidIdx();
    // Reset first valid index found flag
    virtual void ResetFirstValidIdxFlag();
    // Stop decoding at this index and return
    virtual void SetTraceStopIdx(uint64_t index);
    // Ignore packets till this index
    virtual void SetTraceStartIdx(uint64_t index);
    // Get the last valid index
    virtual uint64_t GetLastValidIdx();
    // Get the last sync index
    virtual uint64_t GetLastPEContextIdx();
    // Close the log file
    virtual void CloseLogFile();
    // Reset the first valid index
    virtual void ResetFirstValidIdx();
    // Check if a valid index is found
    virtual bool FirstValidIndexFound();
    // Function to destroy the decoder tree
    virtual void DestroyDecodeTree();
    // Destructor
    virtual ~OpenCSDInterface();
};

// Function pointer to CreateOpenCSDInterface
typedef OpenCSDInterface* (*fpGetOpenCSDInterface)();
// Function pointer to DeleteOpenCSDInterface
typedef void (*fpDeleteOpenCSDInterface)(OpenCSDInterface**);

// Exported API to create and return OpenCSD Class object
extern "C" OPENCSDINTERFACE_API OpenCSDInterface * CreateOpenCSDInterface();
// Exported API to delete OpenCSD class object
extern "C" OPENCSDINTERFACE_API void DeleteOpenCSDInterface(OpenCSDInterface**);

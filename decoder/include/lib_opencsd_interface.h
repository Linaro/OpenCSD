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
    TRACE_DECODER_INIT_ERR,
    TRACE_DECODER_CFG_ERR,
    TRACE_DECODER_MEM_ACC_MAP_CREATE_ERR,
    TRACE_DECODER_MEM_ACC_MAP_ADD_ERR,
    TRACE_DECODER_DATA_PATH_FATAL_ERR,
    TRACE_DECODER_CANNOT_OPEN_FILE,
    TRACE_DECODER_ERR
} TyTraceDecodeError;

//OPENCSDINTERFACE_API OpenCSDTraceDecoder& GetCSDInstance(void);

// Function to initialize the deocder tree
typedef TyTraceDecodeError(*fpInitDecodeTree)(const ocsd_dcd_tree_src_t, const uint32_t);
// Function to initialize the trace output logger
typedef TyTraceDecodeError(*fpInitLogger)(const char *, bool, uint32_t);
// Function to create ETMv4 Decoder
typedef TyTraceDecodeError(*fpCreateETMv4Decoder)(const ocsd_etmv4_cfg);
// Function to create ETMv3 Decoder
typedef TyTraceDecodeError(*fpCreateETMv3Decoder)(const ocsd_etmv3_cfg);
// Function to create STM Decoder
typedef TyTraceDecodeError(*fpCreateSTMDecoder)(const ocsd_stm_cfg);
// Function to create PTM Decoder
typedef TyTraceDecodeError(*fpCreatePTMDecoder)(const ocsd_ptm_cfg);
// Function to create Memory Access Mapper
typedef TyTraceDecodeError(*fpCreateMemAccMapper)();
// Function to add memory access map from bin file
typedef TyTraceDecodeError(*fpAddMemoryAccessMapFromBin)(const ocsd_file_mem_region_t *, const ocsd_mem_space_acc_t, const uint32_t, const char *);
// Function to update memory access map from bin file
typedef TyTraceDecodeError(*fpUpdateMemoryAccessMapFromBin)(const ocsd_file_mem_region_t *, const ocsd_mem_space_acc_t, const uint32_t, const char *);
// Function to add memory access callback
typedef TyTraceDecodeError(*fpAddMemoryAccessCallback)(const ocsd_vaddr_t, const ocsd_vaddr_t, const ocsd_mem_space_acc_t, Fn_MemAcc_CB, const void *);
// Function to decode the trace file
typedef TyTraceDecodeError(*fpDecodeTrace)(const std::string);
// Function to destroy decode tree
typedef void(*fpDestroyDecodeTree)();

#ifdef __cplusplus
extern "C" {
#endif
    // Function to initialize the decoder tree
    OPENCSDINTERFACE_API TyTraceDecodeError InitDecodeTree(const ocsd_dcd_tree_src_t src_type = OCSD_TRC_SRC_FRAME_FORMATTED, const uint32_t formatter_cfg_flags = OCSD_DFRMTR_FRAME_MEM_ALIGN);
    // Function to initialize the trace output logger
    OPENCSDINTERFACE_API TyTraceDecodeError InitLogger(const char *log_file_path, const bool split_files = false, const uint32_t max_rows_in_file = DEAFAULT_MAX_TRACE_FILE_ROW_CNT);
    // Function to create ETMv4 Decoder
    OPENCSDINTERFACE_API TyTraceDecodeError CreateETMv4Decoder(const ocsd_etmv4_cfg config);
    // Function to create ETMv3 Decoder
    OPENCSDINTERFACE_API TyTraceDecodeError CreateETMv3Decoder(const ocsd_etmv3_cfg config);
    // Function to create STM Decoder
    OPENCSDINTERFACE_API TyTraceDecodeError CreateSTMDecoder(const ocsd_stm_cfg config);
    // Function to create PTM Decoder
    OPENCSDINTERFACE_API TyTraceDecodeError CreatePTMDecoder(const ocsd_ptm_cfg config);
    // Function to create Memory Access Mapper
    OPENCSDINTERFACE_API TyTraceDecodeError CreateMemAccMapper();
    // Function to add memory access map from bin file
    OPENCSDINTERFACE_API TyTraceDecodeError AddMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path);
    // Function to update memory access map from bin file
    OPENCSDINTERFACE_API TyTraceDecodeError UpdateMemoryAccessMapFromBin(const ocsd_file_mem_region_t *region, const ocsd_mem_space_acc_t mem_space, const uint32_t num_regions, const char *path);
    // Function to add memory access callback
    OPENCSDINTERFACE_API TyTraceDecodeError AddMemoryAccessCallback(const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void *p_context);
    // Function to decode the trace file
    OPENCSDINTERFACE_API TyTraceDecodeError DecodeOpenCSDTrace(const std::string trace_in_file);
    // Function to destroy decode tree
    OPENCSDINTERFACE_API void DestroyDecodeTree();
#ifdef __cplusplus
}
#endif

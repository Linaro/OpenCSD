/*
 * \file       rctdl_if_types.h
 * \brief      Reference CoreSight Trace Decoder : Standard Types used in the library interfaces.
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

#ifndef ARM_RCTDL_IF_TYPES_H_INCLUDED
#define ARM_RCTDL_IF_TYPES_H_INCLUDED

#include <stdint.h>

/** @defgroup rctdl_interfaces Reference CoreSight Trace Decoder Library : Interfaces
    @brief Set of types, structures and virtual interface classes making up the primary API

  Set of component interfaces that connect various source reader and decode components into a 
  decode tree to allow trace decode for the trace data being output by the source reader.

@{*/

/** @name Library Versioning
@{*/
#define RCTDL_VER_MAJOR 0x0 /**< Library Major Version */
#define RCTDL_VER_MINOR 0x1 /**< Library Minor Version */
#define RCTDL_VER_STRING "0.001"    /**< Library Version string */
#define RCTDL_LIB_NAME "Reference CoreSight Trace Decoder Library"  /**< Library name string */
#define RCTDL_LIB_SHORT_NAME "RCTDL"    /**< Library Short name string */
/** @}*/

/** @name Trace Indexing and Channel IDs
@{*/
#ifdef ENABLE_LARGE_TRACE_SOURCES
typedef uint64_t rctdl_trc_index_t;   /**< Trace source index type - 64 bit size */
#else
typedef uint32_t rctdl_trc_index_t;   /**< Trace source index type - 32 bit size */
#endif

/** Invalid trace index value */
#define RCTDL_BAD_TRC_INDEX           ((rctdl_trc_index_t)-1)
/** Invalid trace source ID value */
#define RCTDL_BAD_CS_SRC_ID           ((uint8_t)-1)
/** macro returing true if trace source ID is in valid range (0x0 < ID < 0x70) */
#define RCTDL_IS_VALID_CS_SRC_ID(id)      ((id > 0) && (id < 0x70))
/** macro returing true if trace source ID is in reserved range (ID == 0x0 || 0x70 <= ID <= 0x7F) */
#define RCTDL_IS_RESERVED_CS_SRC_ID(id)   ((id == 0) || ((id >= 0x70) && (id <= 0x7F))
/** @}*/

/** @name General Library Return and Error Codes 
@{*/

/** Library Error return type */
typedef enum _rctdl_err_t {

    /* general return errors */
    RCTDL_OK = 0,                   /**< No Error. */
    RCTDL_ERR_FAIL,                 /**< General systemic failure. */
    RCTDL_ERR_MEM,                  /**< Internal memory allocation error. */
    RCTDL_ERR_NOT_INIT,             /**< Component not initialised or initialisation failure. */
    RCTDL_ERR_INVALID_ID,           /**< Invalid CoreSight Trace Source ID.  */
    RCTDL_ERR_BAD_HANDLE,           /**< Invalid handle passed to component. */
    RCTDL_ERR_INVALID_PARAM_VAL,    /**< Invalid value parameter passed to component. */
    /* attachment point errors */
    RCTDL_ERR_ATTACH_TOO_MANY,      /**< Cannot attach - attach device limit reached. */
    RCTDL_ERR_ATTACH_INVALID_PARAM, /**< Cannot attach - invalid parameter. */
    RCTDL_ERR_ATTACH_COMP_NOT_FOUND,/**< Cannot detach - component not found. */
    /* source reader errors */
    RCTDL_ERR_RDR_FILE_NOT_FOUND,   /**< source reader - file not found. */
    RCTDL_ERR_RDR_INVALID_INIT,     /**< source reader - invalid initialisation parameter. */
    RCTDL_ERR_RDR_NO_DECODER,       /**< source reader - not trace decoder set. */
    /* data path errors */
    RCTDL_ERR_DATA_DECODE_FATAL,    /**< A decoder in the data path has returned a fatal error. */
    /* frame deformatter errors */
    RCTDL_ERR_DFMTR_NOTCONTTRACE,    /**< Trace input to deformatter none-continuous */
    /* packet processor errors - protocol issues etc */
    /* packet decoder errors */
    /* test errors - errors generated only by the test code, not the library */
    RCTDL_ERR_TEST_SNAPSHOT_PARSE,       /**< test snapshot file parse error */
    RCTDL_ERR_TEST_SNAPSHOT_PARSE_INFO,  /**< test snapshot file parse information */
    RCTDL_ERR_TEST_SNAPSHOT_READ,        /**< test snapshot reader error */
    RCTDL_ERR_TEST_SS_TO_DECODER,        /**< test snapshot to decode tree conversion error */
    /* end marker*/
    RCTDL_ERR_LAST
} rctdl_err_t;

/* component handle types */
typedef int rctdl_hndl_rdr_t;               /**< reader control handle */
typedef unsigned int rctdl_hndl_err_log_t;  /**< error logger connection handle */

/* common invalid handle type */
#define RCTDL_INVALID_HANDLE -1     /**< Global invalid handle value */

/*!  Error Severity Type
 * 
 *   Used to indicate the severity of an error, and also as the 
 *   error log verbosity level in the error logger.
 *   
 *   The logger will ignore errors with a severity value higher than the 
 *   current verbosity level.
 *
 *   The value RCTDL_ERR_SEV_NONE can only be used as a verbosity level to switch off logging,
 *   not as a severity value on an error. The other values can be used as both error severity and
 *   logger verbosity values.
 */
typedef enum _rctdl_err_severity_t {
    RCTDL_ERR_SEV_NONE,     /**< No error logging. */
    RCTDL_ERR_SEV_ERROR,    /**< Most severe error - perhaps fatal. */
    RCTDL_ERR_SEV_WARN,     /**< Warning level. Inconsistent or incorrect data seen but can carry on decode processing */
    RCTDL_ERR_SEV_INFO,     /**< Information only message. Use for debugging code or suspect input data. */
} rctdl_err_severity_t;

/** @}*/

/** @name Trace Datapath 
@{*/

/** Trace Datapath operations.
  */
typedef enum _rctdl_datapath_op_t {
    RCTDL_OP_DATA = 0, /**< Standard index + data packet */
    RCTDL_OP_EOT,   /**< End of available trace data. No data packet. */
    RCTDL_OP_FLUSH, /**< Flush existing data where possible, retain decode state. No data packet. */
    RCTDL_OP_RESET, /**< Reset decode state - drop any existing partial data. No data packet. */
} rctdl_datapath_op_t;

/**
  * Trace Datapath responses
  */
typedef enum _rctdl_datapath_resp_t {
    RCTDL_RESP_CONT,                /**< Continue processing */
    RCTDL_RESP_WARN_CONT,           /**< Continue processing  : a component logged a warning. */
    RCTDL_RESP_ERR_CONT,            /**< Continue processing  : a component logged an error.*/
    RCTDL_RESP_WAIT,                /**< Pause processing */
    RCTDL_RESP_WARN_WAIT,           /**< Pause processing : a component logged a warning. */
    RCTDL_RESP_ERR_WAIT,            /**< Pause processing : a component logged an error. */
    RCTDL_RESP_FATAL_NOT_INIT,      /**< Processing Fatal Error :  component unintialised. */
    RCTDL_RESP_FATAL_INVALID_OP,    /**< Processing Fatal Error :  invalid data path operation. */
    RCTDL_RESP_FATAL_INVALID_PARAM, /**< Processing Fatal Error :  invalid parameter in datapath call. */
    RCDTL_REST_FATAL_INVALID_DATA,  /**< Processing Fatal Error :  invalid trace data */
    RCTDL_RESP_FATAL_SYS_ERR,       /**< Processing Fatal Error :  internal system error. */
} rctdl_datapath_resp_t;

/*! Macro returning true if response value is FATAL. */
#define RCTDL_DATA_RESP_IS_FATAL(x) (x >= RCTDL_RESP_FATAL_NOT_INIT)
/*! Macro returning true if response value indicates WARNING logged. */
#define RCTDL_DATA_RESP_IS_WARN(x) ((x == RCTDL_RESP_WARN_CONT) || (x == RCTDL_RESP_WARN_WAIT))
/*! Macro returning true if response value indicates ERROR logged. */
#define RCTDL_DATA_RESP_IS_ERR(x) ((x == RCTDL_RESP_ERR_CONT) || (x == RCTDL_RESP_ERR_WAIT))
/*! Macro returning true if response value indicates WARNING or ERROR logged. */
#define RCTDL_DATA_RESP_IS_WARN_OR_ERR(x) (RCTDL_DATA_RESP_IS_ERR(x) || RCTDL_DATA_RESP_IS_WARN(x))

#define RCTDL_DATA_RESP_IS_CONT(x) (x <  RCTDL_RESP_WAIT)

#define RCTDL_DATA_RESP_IS_WAIT(x) ((x >= RCTDL_RESP_WAIT) && (x < RCTDL_RESP_FATAL_NOT_INIT))

/** @}*/

/** @name Trace Decode component types 
@{*/

/*! Trace Protocol Types - used to create appropriate decoder.
 */
typedef enum _rctdl_trace_protocol_t {
    RCTDL_PROTOCOL_EXTERN, /**< Custom external decoder attached to the decode tree - protocol unknown */
    RCTDL_PROTOCOL_ETMV3,
    RCTDL_PROTOCOL_ETMV4I,
    RCTDL_PROTOCOL_ETMV4D,
    RCTDL_PROTOCOL_PTM,
    /* others to be added here */
    RCTDL_PROTOCOL_END
} rctdl_trace_protocol_t;


/** Raw frame element data types 
    Data blocks types output from ITrcRawFrameIn. 
*/
typedef enum _rcdtl_rawframe_elem_t {
    RCTDL_FRM_NONE,     /**< None data operation on data path. (EOT etc.) */
    RCTDL_FRM_PACKED,   /**< Raw packed frame data */
    RCTDL_FRM_HSYNC,    /**< HSYNC data */
    RCTDL_FRM_FSYNC,    /**< Frame Sync Data */
    RCTDL_FRM_ID_DATA,  /**< unpacked data for ID */
    RCTDL_FRM_ID_CHANGE, /**< ID change data */
} rctdl_rawframe_elem_t;


/** Indicates if the trace source will be frame formatted or a single protocol source.
    Used in decode tree creation and configuration code.
*/
typedef enum _rctdl_dcd_tree_src_t {
    RCTDL_TRC_SRC_FRAME_FORMATTED,  /**< input source is frame formatted. */
    RCTDL_TRC_SRC_SINGLE,           /**< input source is from a single protocol generator. */
} rctdl_dcd_tree_src_t;

#define RCTDL_DFRMTR_HAS_FSYNCS      0x1 /**< formatted data has fsyncs - input data 4 byte aligned */
#define RCTDL_DFRMTR_HAS_HSYNCS      0x2 /**< formatted data has hsyncs - input data 2 byte aligned */
#define RCTDL_DFRMTR_FRAME_MEM_ALIGN 0x4 /**< formatted frames are memory aligned, no syncs. Input data 16 byte frame aligned. */
#define RCTDL_DFRMTR_VALID_MASK      0x7 /**< valid mask for deformatter configuration */
#define RCTDL_DFRMTR_FRAME_SIZE      0x10 /**< CoreSight frame formatter frame size in bytes. */

/** @}*/

/** @name Trace Decode Component Name Prefixes 
 *
 *  Set of standard prefixes to be used for component names
@{*/

/** Component name prefix for trace source reader components */
#define RCTDL_CMPNAME_PREFIX_SOURCE_READER "SRDR"
/** Component name prefix for trace frame deformatter component */
#define RCTDL_CMPNAME_PREFIX_FRAMEDEFORMATTER "DFMT"
/** Component name prefix for trace packet processor. */
#define RCTDL_CMPNAME_PREFIX_PKTPROC "PKTP"
/** Component name prefix for trace packet decoder. */
#define RCTDL_CMPNAME_PREFIX_PKTDEC   "PDEC"

/** @}*/

/** @name Trace Decode Arch and Profile 
@{*/

/** Core Architecture Version */
typedef enum _rctdl_arch_version {
    ARCH_UNKNOWN,   /**< unknown architecture */
    ARCH_V7,        /**< V7 architecture */
    ARCH_V8         /**< V8 architecture */
} rctdl_arch_version_t;

/** Core Profile  */
typedef enum _rctdl_core_profile {
    profile_Unknown,    /**< Unknown profile */
    profile_CortexM,    /**< Cortex-M profile */
    profile_CortexR,    /**< Cortex-R profile */
    profile_CortexA     /**< Cortex-A profile */
} rctdl_core_profile_t;

/** Combined architecture and profile descriptor for a core */
typedef struct _rctdl_arch_profile_t {
    rctdl_arch_version_t arch;      /**< core architecture */
    rctdl_core_profile_t profile;   /**< core profile */
} rctdl_arch_profile_t;

/** may want to use a 32 bit v-addr when running on 32 bit only ARM platforms. */
#ifdef USE_32BIT_V_ADDR
typedef uint32_t rctdl_vaddr_t;
#define RCTDL_MAX_VA_BITSIZE 32
#define RCTDL_VA_MASK ~0UL
#else
typedef uint64_t rctdl_vaddr_t;
#define RCTDL_MAX_VA_BITSIZE 64
#define RCTDL_VA_MASK ~0ULL
#endif

/** A bit mask for the first 'bits' consecutive bits of an address */ 
#define RCTDL_BIT_MASK(bits) (bits == RCTDL_MAX_VA_BITSIZE) ? RCTDL_VA_MASK : ((rctdl_vaddr_t)1 << bits) - 1



/** @}*/

/** @name Instruction Decode Information
@{*/

/** Instruction Set Architecture 
 *
 */
typedef enum _rctdl_isa
{    
    rctdl_isa_arm,          /**< V7 ARM */  
    rctdl_isa_t16,          /**< Thumb 16 */  
    rctdl_isa_tee,          /**< Thumb EE */  
    rctdl_isa_jazelle,      /**< Jazelle */  
    rctdl_isa_aarch32,      /**< V8 AArch32 */
    rctdl_isa_aarch32t,     /**< V8 AArch32 Thumb */
    rctdl_isa_aarch64,      /**< V8 AArch64 */
    rctdl_isa_unknown       /**< ISA not yet known */
} rctdl_isa;

/** Security level
*/
typedef enum _rctdl_sec_level
{
    rctdl_sec_secure,   /**< Core is in secure state */
    rctdl_sec_nonsecure /**< Core is in non-secure state */
} rctdl_sec_level ;

/** Exception level 
*/
typedef enum _rctdl_ex_level
{
    rctdl_EL0,
    rctdl_EL1,
    rctdl_EL2,
    rctdl_EL3,
} rctdl_ex_level;


/** instruction type - TBC */
typedef enum _rctdl_instr_type {
    RCTDL_INSTR_OTHER,          /**< Other instruction - not significant for waypoints. */
    RCTDL_INSTR_BR,             /**< Branch instruction */
    RCTDL_INSTR_BR_INDIRECT,    /**< Indirect Branch instruction */
    RCTDL_INSTR_DSB_ISB,        /**< DSB or ISB instruction */
    RCTDL_INSTR_ERET            /**< Exception return */
} rctdl_instr_type;

/** Instruction decode request structure. 
 *
 *   Used in IInstrDecode  interface.
 *
 *   Caller fills in the input: information, callee then fills in the decoder: information.
 */
typedef struct _rctdl_instr_info {
    /* input information */
    rctdl_arch_version_t arch;      /**< Input: Core architecture. */
    rctdl_core_profile_t profile;   /**< Input: Core profile. */
    rctdl_isa isa;                  /**< Input: Current ISA. */
    rctdl_vaddr_t instr_addr;       /**< Input: Instruction address. */
    uint32_t opcode;                /**< Input: Opcode at address. 16 bit opcodes will use LS 16bits of parameter. */

    /* instruction decode info */
    rctdl_instr_type type;          /**< Decoder: Current instruction type. */
    rctdl_vaddr_t next_addr;        /**< Decoder: Instruction address for next instruction. */
    rctdl_isa next_isa;             /**< Decoder: ISA for next intruction. */

} rctdl_instr_info;


/** @}*/

/** @name Packet Processor Operation Control Flags
@{*/

#define RCTDL_PKTPROC_FLG_NOFWD_BAD_PKTS 0x00000001  /**< don't forward bad packets up data path */
#define RCTDL_PKTPROC_FLG_NOMON_BAD_PKTS 0x00000002  /**< don't forward bad packets to monitor interface */

/** @}*/


/** @}*/
#endif // ARM_RCTDL_IF_TYPES_H_INCLUDED

/* End of File rctdl_if_types.h */

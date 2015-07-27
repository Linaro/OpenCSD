/*
 * \file       trc_pkt_types_etmv3.h
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

#ifndef ARM_TRC_ETM3_PKT_TYPES_ETMV3_H_INCLUDED
#define ARM_TRC_ETM3_PKT_TYPES_ETMV3_H_INCLUDED

#include "trc_pkt_types.h"

/** @addtogroup trc_pkts
@{*/

/** @name ETMv3 Packet Types
@{*/

typedef enum _rctdl_etmv3_pkt_type
{

// markers for unknown/bad packets
        ETM3_PKT_NOERROR,        //!< no error in packet - supplimentary data.
		ETM3_PKT_NOTSYNC,        //!< no sync found yet
		ETM3_PKT_BAD_SEQUENCE,   //!< invalid sequence for packet type
        ETM3_PKT_BAD_TRACEMODE,  //!< invalid packet type for this trace mode.
		ETM3_PKT_RESERVED,       //!< packet type reserved.
        ETM3_PKT_INCOMPLETE_EOT, //!< flushing incomplete/empty packet at end of trace.

// markers for valid packets
		ETM3_PKT_BRANCH_ADDRESS,
		ETM3_PKT_A_SYNC,
		ETM3_PKT_CYCLE_COUNT,
		ETM3_PKT_I_SYNC,
		ETM3_PKT_I_SYNC_CYCLE,
        ETM3_PKT_TRIGGER,
        ETM3_PKT_P_HDR,
		ETM3_PKT_STORE_FAIL,
		ETM3_PKT_OOO_DATA,
		ETM3_PKT_OOO_ADDR_PLC,
		ETM3_PKT_NORM_DATA,
		ETM3_PKT_DATA_SUPPRESSED,
		ETM3_PKT_VAL_NOT_TRACED,
		ETM3_PKT_IGNORE,
		ETM3_PKT_CONTEXT_ID,
        ETM3_PKT_VMID,
		ETM3_PKT_EXCEPTION_ENTRY,
		ETM3_PKT_EXCEPTION_EXIT,		
		ETM3_PKT_TIMESTAMP,

// internal processing types
		ETM3_PKT_BRANCH_OR_BYPASS_EOT,
        ETM3_PKT_AUX_DATA

} rctdl_etmv3_pkt_type;

typedef struct _rctdl_etmv3_excep {
    rctdl_armv7_exception type; /**<  exception type. */
    uint16_t number;    /**< exception as number */
    struct {
    int present:1;      /**< exception present in packet */
    int cancel:1;       /**< exception cancels prev instruction traced. */
    int cm_resume:4;    /**< M class resume code */
    int cm_irq_n:9;     /**< M class IRQ n */
    } bits;
} rctdl_etmv3_excep;


typedef struct _rctdl_etmv3_pkt
{
    rctdl_etmv3_pkt_type type;  /**< Primary packet type. */

    rctdl_isa curr_isa;         /**< current ISA */
    rctdl_isa prev_isa;         /**< ISA in previous packet */

    struct {
    int      curr_alt_isa:1;     /**< current Alt ISA flag for Tee / T16 (used if not in present packet) */
    int      curr_NS:1;          /**< current NS flag  (used if not in present packet) */
    int      curr_Hyp:1;         /**< current Hyp flag  (used if not in present packet) */
    } bits;

    rctdl_pkt_vaddr addr;       /**< current Addr */

    rctdl_etmv3_excep exception;
    
    
    //rctdl_pkt_byte_sz_val by_sz_val;        /**< byte sized value - ContextID, VMID - always whole num of bytes. */
    //rctdl_pkt_atom  atom;

    //uint32_t cycle_count;
    //uint64_t timestamp;

    rctdl_etmv3_pkt_type err_type;  /**< Basic packet type if primary type indicates error or incomplete. (header type) */

} rctdl_etmv3_pkt;

typedef struct _rctdl_etmv3_cfg 
{
    uint32_t                reg_idr;    /**< ID register */
    uint32_t                reg_ctrl;   /**< Control Register */
    uint32_t                reg_ccer;   /**< CCER register */
    uint32_t                reg_trc_id; /**< Trace Stream ID register */
    rctdl_arch_version_t    arch_ver;   /**< Architecture version */
    rctdl_core_profile_t    core_prof;  /**< Core Profile */
} rctdl_etmv3_cfg;

/** @}*/
/** @}*/
#endif // ARM_TRC_ETM3_PKT_TYPES_ETMV3_H_INCLUDED

/* End of File trc_pkt_types_etmv3.h */

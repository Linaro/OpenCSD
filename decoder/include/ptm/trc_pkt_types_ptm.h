/*
 * \file       trc_pkt_ptm_types.h
 * \brief      Reference CoreSight Trace Decoder : PTM specific types
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

#ifndef ARM_TRC_PKT_PTM_TYPES_H_INCLUDED
#define ARM_TRC_PKT_PTM_TYPES_H_INCLUDED

#include "trc_pkt_types.h"

/** @addtogroup trc_pkts
@{*/

/** @name PTM Packet Types
@{*/

typedef enum _rctdl_ptm_pkt_type
{
// markers for unknown / bad packets
	PTM_PKT_NOTSYNC,        //!< no sync found yet
   	PTM_PKT_BAD_SEQUENCE,   //!< invalid sequence for packet type
	PTM_PKT_RESERVED,		//!< Reserved packet encoding	
    PTM_PKT_INCOMPLETE_EOT, //!< flushing incomplete packet at end of trace.
    PTM_PKT_NOERROR,        //!< no error base type packet.
// markers for valid packets
    PTM_PKT_BRANCH_ADDRESS, //!< Branch address with optional exception.	 
    PTM_PKT_A_SYNC,			//!< Alignment Synchronisation.
	PTM_PKT_I_SYNC,			//!< Instruction sync with address.
	PTM_PKT_TRIGGER,		//!< trigger packet
	PTM_PKT_WPOINT_UPDATE,  //!< Waypoint update. 
	PTM_PKT_IGNORE,			//!< ignore packet.
	PTM_PKT_CONTEXT_ID,		//!< context id packet.
	PTM_PKT_ATOM,			//!< atom waypoint packet.
	PTM_PKT_TIMESTAMP,		//!< timestamp packet.
	PTM_PKT_EXCEPTION_RET,	//!< exception return.
	PTM_PKT_BRANCH_OR_BYPASS_EOT, // interpreter FSM 'state' : unsure if branch 0 packet or bypass flush end of trace
    PTM_PKT_TPIU_PAD_EOB,   // pad end of a buffer - no valid trace at this point
} rctdl_ptm_pkt_type;

typedef struct _rctdl_ptm_pkt
{
    rctdl_ptm_pkt_type type;  /**< Primary packet type. */
    rctdl_ptm_pkt_type err_type;  /**< Basic packet type if primary type indicates error or incomplete. */

    rctdl_pkt_vaddr addr;
    rctdl_pkt_byte_sz_val by_sz_val;
    rctdl_pkt_atom  atom;

    uint32_t cycle_count;
    uint64_t timestamp;

} rctdl_ptm_pkt;

typedef struct _rctdl_ptm_cfg 
{
    uint32_t                reg_idr;    /**< PTM ID register */
    uint32_t                reg_ctrl;   /**< Control Register */
    uint32_t                reg_ccer;   /**< Condition code extension register */
    uint32_t                reg_trc_id; /**< CoreSight Trace ID register */
    rctdl_arch_version_t    arch_ver;   /**< Architecture version */
    rctdl_core_profile_t    core_prof;  /**< Core Profile */
} rctdl_ptm_cfg;

/** @}*/


/** @}*/
#endif // ARM_TRC_PKT_PTM_TYPES_H_INCLUDED

/* End of File trc_pkt_ptm_types.h */

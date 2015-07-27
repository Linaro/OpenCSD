/*
 * \file       rctdl_c_api_types.h
 * \brief      Reference CoreSight Trace Decoder : Trace Decoder "C" API types.
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

#ifndef ARM_RCTDL_C_API_TYPES_H_INCLUDED
#define ARM_RCTDL_C_API_TYPES_H_INCLUDED

/* select the library types that are C compatible - the interface data types */
#include "rctdl_if_types.h"
#include "trc_gen_elem_types.h"
#include "trc_pkt_types.h"


/* pull in the protocol decoder types. */
#include "etmv3/trc_pkt_types_etmv3.h"
#include "etmv4/trc_pkt_types_etmv4.h"
#include "ptm/trc_pkt_types_ptm.h"

/** @ingroup lib_c_api
@{*/


/* Specific C-API only types */

/** Handle to decode tree */
typedef void * dcd_tree_handle_t;


typedef rctdl_datapath_resp_t (* FnTraceElemIn)(const rctdl_trc_index_t index_sop, const uint8_t trc_chan_id, const rctdl_generic_trace_elem *elem); 

/** @}*/


#endif // ARM_RCTDL_C_API_TYPES_H_INCLUDED

/* End of File rctdl_c_api_types.h */

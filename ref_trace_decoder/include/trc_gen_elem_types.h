/*!
 * \file       trc_gen_elem_types.h
 * \brief      Reference CoreSight Trace Decoder : Decoder Output Generic Element types.
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

#ifndef ARM_TRC_GEN_ELEM_TYPES_H_INCLUDED
#define ARM_TRC_GEN_ELEM_TYPES_H_INCLUDED

/** @defgroup gen_trc_elem  Reference CoreSight Trace Decoder Library : Generic Trace Elements
  * @brief Generic trace elements output by the PE trace decode and SW stim decode stages.
  *
  * 
@{*/

#include "rctdl_if_types.h"

/**  Enum for generic element types */
typedef enum _rctdl_gen_trc_elem_t 
{  
    RCTDL_GEN_TRC_ELEM_UNKNOWN = 0,     /*!< Unknown trace element - default value or indicate error in stream to client */
    RCTDL_GEN_TRC_ELEM_NO_SYNC,         /*!< Waiting for sync - either at start of decode, or after overflow / bad packet */
    RCTDL_GEN_TRC_ELEM_TRACE_ON,        /*!< Start of trace - beginning of elements or restart after discontinuity (overflow, trace filtering). */
    RCTDL_GEN_TRC_ELEM_EO_TRACE,        /*!< end of the available trace in the buffer.  */
    RCTDL_GEN_TRC_ELEM_PE_CONTEXT,      /*!< PE status update / change (arch, ctxtid, vmid etc).  */
    RCTDL_GEN_TRC_ELEM_INSTR_RANGE,     /*!< traced N consecutive instructions from addr (no intervening events or data elements), may have data assoc key  */
    RCTDL_GEN_TRC_ELEM_ADDR_NACC,       /*!< tracing in inaccessible memory area  */ 
    RCTDL_GEN_TRC_ELEM_EXCEPTION,       /*!< exception - trace range executed up to an exception occurring */
    RCTDL_GEN_TRC_ELEM_EXCEPTION_RET,   /*!< expection return */
    RCTDL_GEN_TRC_ELEM_TIMESTAMP,       /*!< Timestamp - preceding elements happeded before this time. */
    RCTDL_GEN_TRC_ELEM_CYCLE_COUNT,     /*!< Cycle count - cycles since last cycle count value - associated with a preceding instruction range. */
    RCTDL_GEN_TRC_ELEM_EVENT,           /*!< Event - trigger, (TBC - perhaps have a set of event types - cut down additional processing?)  */
#if 0
    RCTDL_GEN_TRC_ELEM_DATA_VAL,        /*!< Data value - associated with prev instr (if same stream) + daddr, or data assoc key if supplied.  */
    RCTDL_GEN_TRC_ELEM_DATA_ADDR,       /*!< Data address - associated with prev instr (if same stream), or data assoc key if supplied.  */
    RCTDL_GEN_TRC_ELEM_SWCHAN_DATA,     /*!< data out on a SW channel (master, ID, data, type etc).  */
    RCTDL_GEN_TRC_ELEM_BUS_TRANSFER,    /*!< Bus transfer event from a bus trace module (HTM)  */
#endif
    
} rctdl_gen_trc_elem_t;


typedef enum _trace_on_reason_t {
    TRACE_ON_NORMAL = 0,    /**< Trace on at start of trace or filtering discontinuity */
    TRACE_ON_OVERFLOW,      /**< Trace on due to prior trace overflow discontinuity */
    TRACE_ON_EX_DEBUG,      /**< Trace restarted due to debug exit */
} trace_on_reason_t;


typedef struct _rctdl_generic_trace_elem {
    rctdl_gen_trc_elem_t elem_type;   /**< Element type - remaining data interpreted according to this value */
    rctdl_isa           isa;          /**< instruction set for executed instructions */
    rctdl_vaddr_t       st_addr;      /**< start address for instruction execution range / inaccessible code address / data address */
    rctdl_vaddr_t       en_addr;      /**< end address (exclusive) for instruction execution range. */
    rctdl_pe_context    context;      /**< PE Context */
    uint64_t            timestamp;    /**< timestamp value for TS element type */
    uint32_t            cycle_count;  /**< cycle count for explicit cycle count element, or count for element with associated cycle count */
    rctdl_instr_type    last_i_type;  /**< Last instruction type if instruction execution range */
 
    // per element flags
    struct {
        uint32_t last_instr_exec:1;     /**< 1 if last instruction in range was executed; */
        uint32_t has_cc:1;              /**< 1 if this packet has a valid cycle count included (e.g. cycle count included as part of instruction range packet, always 1 for pure cycle count packet.*/
        uint32_t cpu_freq_change:1;     /**< 1 if this packet indicates a change in CPU frequency */
        uint32_t i_type_with_link:1;    /**< 1 if last_i_type branch is branch with link */
    };

    union {
        uint32_t gen_value;    /**< general value for simpler types of element. */    
        uint32_t exception_number; /**< exception number for exception type packets */
        struct trace_event_t {
            uint16_t ev_type;          /**< event type - unknown (0) trigger (1), numbered event (2)*/
            uint16_t ev_number;        /**< event number if numbered event type */
        } trace_event;
        trace_on_reason_t trace_on_reason;  /**< reason for the trace on packet */
    };

} rctdl_generic_trace_elem;


typedef enum _event_t {
    EVENT_UNKNOWN = 0,
    EVENT_TRIGGER,
    EVENT_NUMBERED
} event_t;


/** @}*/
#endif // ARM_TRC_GEN_ELEM_TYPES_H_INCLUDED

/* End of File trc_gen_elem_types.h */

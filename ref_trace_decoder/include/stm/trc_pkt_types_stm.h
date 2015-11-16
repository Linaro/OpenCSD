/*
 * \file       trc_pkt_types_stm.h
 * \brief      Reference CoreSight Trace Decoder : STM decoder
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
#ifndef ARM_TRC_PKT_TYPES_STM_H_INCLUDED
#define ARM_TRC_PKT_TYPES_STM_H_INCLUDED

#include "trc_pkt_types.h"

/** @addtogroup trc_pkts
@{*/

/** @name STM Packet Types
@{*/

typedef enum _rctdl_stm_pkt_type
{
    STM_PKT_NOTSYNC,
    STM_PKT_INCOMPLETE_EOT,
    STM_PKT_NO_ERR_TYPE,

    STM_PKT_BAD_SEQUENCE,
    STM_PKT_RESERVED,

    STM_PKT_ASYNC,
    STM_PKT_VERSION,
    STM_PKT_FREQ,
    STM_PKT_NULL,
    STM_PKT_TRIG,

    STM_PKT_GERR,
    STM_PKT_MERR,

    STM_PKT_M8,
    STM_PKT_C8,
    STM_PKT_C16,

    STM_PKT_FLAG,

    STM_PKT_D8,
    STM_PKT_D16,
    STM_PKT_D32,
    STM_PKT_D64

} rctdl_stm_pkt_type;


typedef enum _rctdl_stm_ts_type
{
    STM_TS_UNKNOWN,
    STM_TS_NATBINARY,
    STM_TS_GREY
} rctdl_stm_ts_type;

typedef struct _rctdl_stm_pkt
{
    rctdl_stm_pkt_type type;

    uint8_t     master;             /**< current master */
    uint16_t    channel;            /**< current channel */
    
    uint64_t    timestamp;          /**< latest ts value */
    uint8_t     pkt_ts_bits;        /**< ts bits updated this packet */
    rctdl_stm_ts_type ts_type;      /**< ts encoding type */

    uint8_t     pkt_has_marker;     /**< flag to indicate marker packet */

    union {
        uint8_t  D8;    /**< payload for D8 data packet, or parameter value for other packets with 8 bit value [VERSION, TRIG, xERR] */
        uint16_t D16;   /**< payload for D16 data packet */
        uint32_t D32;   /**< payload for D32 data packet, or parameter value for other packets with 32 bit value [FREQ] */
        uint64_t D64;   /**< payload for D64 data packet */
    } payload;

    rctdl_stm_pkt_type err_type;

} rctdl_stm_pkt;


typedef struct _rctdl_stm_cfg
{
    uint32_t reg_tcsr;      /**< Contains CoreSight trace ID */
    uint32_t reg_feat3r;    /**< defines number of masters */
    uint32_t reg_devid;     /**< defines number of channels per master */

} rctdl_stm_cfg;

/** @}*/
/** @}*/

#endif // ARM_TRC_PKT_TYPES_STM_H_INCLUDED

/* End of File trc_pkt_types_stm.h */

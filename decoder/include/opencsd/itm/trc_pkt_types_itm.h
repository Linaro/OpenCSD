/*
 * \file       trc_pkt_types_itm.h
 * \brief      OpenCSD : ITM decoder
 *
 * \copyright  Copyright (c) 2024, ARM Limited. All Rights Reserved.
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

#ifndef ARM_TRC_PKT_TYPES_ITM_H_INCLUDED
#define ARM_TRC_PKT_TYPES_ITM_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>

typedef enum _ocsd_itm_pkt_type {
/* markers for unknown packets  / state*/
	ITM_PKT_NOTSYNC,            /**< Not synchronised */
	ITM_PKT_INCOMPLETE_EOT,     /**< Incomplete packet flushed at end of trace. */
	ITM_PKT_NO_ERR_TYPE,        /**< No error in error packet marker. */

/* valid packet types */
	ITM_PKT_ASYNC,				/**< sync packet */
	ITM_PKT_OVERFLOW,			/**< overflow packet */
	ITM_PKT_SWIT,				/**< Software stimulus packet */
	ITM_PKT_DWT,				/**< DWT hardware stimulus packet */
	ITM_PKT_TS_LOCAL,			/**< Timestamp packet using local timestamp source */
	ITM_PKT_TS_GLOBAL_1,		/**< Timestamp packet bits [25:0] from the global timestamp source */
	ITM_PKT_TS_GLOBAL_2,		/**< Timestamp packet bits [63:26] or [47:26] from the global timestamp source */
	ITM_PKT_EXTENSION,			/**< Extension packet */

/* packet errors */
	ITM_PKT_BAD_SEQUENCE,
	ITM_PKT_RESERVED,

} ocsd_itm_pkt_type;

/* incoming ITM packet*/
typedef struct _ocsd_itm_pkt {
	ocsd_itm_pkt_type type;			/**< ITM packet type */
	/**! Source ID uses: 
		 - SWIT: value of source channel [4:0],
	     - DWT: value of discriminator   [4:0], 
		 - LTS: TC flags for Local TS pkt [1:0],
		 - GTS1: clk wrap [1] / freq change [0] bits,
		 - Ext: Src SW(0)/HW(1) [7], N size - N:0 value bit length [4:0],
	*/
	uint8_t	 src_id;				
	uint32_t value;					/**< packet data payload - interpretation depends on type */
	uint8_t  val_sz;				/**< size of value in bytes */
	uint8_t  val_ext;				/**< additional value bits to handle top of [63:26] timestamp packet (38 bits of ts) */
	ocsd_itm_pkt_type err_type;		/**< Initial type of packet if type indicates bad sequence. */
} ocsd_itm_pkt;

/** ITM hardware configuration.
 * Contains control register value
 */
typedef struct _ocsd_itm_cfg
{
	uint32_t reg_tcr;          /**< Contains CoreSight trace ID, TS prescaler */
} ocsd_itm_cfg;

typedef enum _ocsd_itm_dwt_ecntr {
	ITM_DWT_ECNTR_CPI = 0x01,
	ITM_DWT_ECNTR_EXC = 0x02,
	ITM_DWT_ECNTR_SLP = 0x04, 
	ITM_DWT_ECNTR_LSU = 0x08, 
	ITM_DWT_ECNTR_FLD = 0x10,
	ITM_DWT_ECNTR_CYC = 0x20,
} ocsd_itm_dwt_ecntr;

#endif  // ARM_TRC_PKT_TYPES_ITM_H_INCLUDED

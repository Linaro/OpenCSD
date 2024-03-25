/*!
 * \file       itm_test_gen.h
 * \brief      OpenCSD : Test code for ITM decoder
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

#ifndef ARM_ITM_TEST_GEN_H_INCLUDED
#define ARM_ITM_TEST_GEN_H_INCLUDED


/* generator macros and key values for ITM test data */

  /* ITM SWIT / DWT size field values */
#define ITM_SWIT_SZ_8_BIT	0x1
#define ITM_SWIT_SZ_16_BIT	0x2
#define ITM_SWIT_SZ_32_BIT	0x3

/* LTS TC values */
#define ITM_LTS_TC_TS_SYNC_FLAG				0x0
#define ITM_LTS_TC_TS_DELAY_FLAG			0x1
#define ITM_LTS_TC_PKT_DELAY_FLAG			0x2
#define ITM_LTS_TC_TS_AND_PKT_DELAY_FLAG	0x3

/* DWT data comparator number values */
#define ITM_DWT_CMPN_0	0x0
#define ITM_DWT_CMPN_1	0x1
#define ITM_DWT_CMPN_2	0x2
#define ITM_DWT_CMPN_3	0x3

/* Helper macros for generating test data */

// generate the SWIT channel and size header byte 
#define SWIT_CHAN(chan, size) ((((uint8_t)chan & 0x1F) << 3) | (size & 0x3))

// hardware DWT packet header - takes discriminator id value and size
#define DWT_HEADER(disc_id, size) ((((uint8_t)disc_id & 0x1F) << 3) | 0x04 | (size & 0x3))

#define DWT_EXCEP_FN_ENTER  0x1
#define DWT_EXCEP_FN_EXIT   0x2
#define DWT_EXCEP_FN_RETURN 0x3

// generate discriminator ID values for DWT data trace
#define DWT_DT_DISC_ID_PC(cmpn) ((((uint8_t)(cmpn) & 0x3) << 1) | 0x4)
#define DWT_DT_DISC_ID_ADDR(cmpn) ((((uint8_t)(cmpn) & 0x3) << 1) | 0x5)
#define DWT_DT_DISC_ID_DATA_R(cmpn) ((((uint8_t)(cmpn) & 0x3) << 1) | 0x10)
#define DWT_DT_DISC_ID_DATA_W(cmpn) ((((uint8_t)(cmpn) & 0x3) << 1) | 0x11)


// convert values into 1, 2 or 4 comma separated values
#define VAL_8_BIT(val)  (((uint8_t)(val)) & 0xFF)
#define VAL_16_BIT(val) VAL_8_BIT(val), VAL_8_BIT(val >> 8)
#define VAL_32_BIT(val) VAL_16_BIT(val), VAL_16_BIT(val >> 16)

// Local timestamp header and value bytes
#define TS_CONT 0x80
#define TS_TC_BIT 0x40
#define TS_HDR_TC(flag) ((((uint8_t)(flag) & 0x3) << 4) | TS_CONT | TS_TC_BIT)
#define TS_HDR_TS(val) (((uint8_t)(val) & 0x7) << 4)
#define TS_7_BITS(val) ((uint8_t)(val) & 0x7F)
#define TS_7_BITS_CONT(val)  (TS_7_BITS(val) | TS_CONT)

// global timestamp values
#define GTS1_HEADER() 0x94
#define GTS2_HEADER() 0xB4
#define GTS1_BYTE3(time, wrap, clk_ch) (((time >> 21) & 0x1F) | ((wrap & 0x1) << 6) |  ((clk_ch & 0x1) << 5))

/*** macros to generate static ITM test data
   - GEN_ generators expand to comma separated 8 bit values
   - all generators end in a ',' to allow them to be concatenated to 
     build a test data array.
 */

/*! GEN_ASYNC(): generate the async packet
 */
#define GEN_ASYNC() 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,

/*! GEN_OVERFLOW(): generate an overflow packet
 */
#define GEN_OVERFLOW() 0x70,

/*! GEN_SWIT_<n>_BYTE(chan, val): set of generators to create 1, 2 and 4 byte SWT packets.
 *  chan - the channel number / bits 6:2 of the stimulus write address.
 *  val  - value to encode into the packet
 */
#define GEN_SWIT_1_BYTE(chan, val) SWIT_CHAN(chan, ITM_SWIT_SZ_8_BIT), VAL_8_BIT(val),
#define GEN_SWIT_2_BYTE(chan, val) SWIT_CHAN(chan, ITM_SWIT_SZ_16_BIT), VAL_16_BIT(val),
#define GEN_SWIT_4_BYTE(chan, val) SWIT_CHAN(chan, ITM_SWIT_SZ_32_BIT), VAL_32_BIT(val),

/* DWT packet generators - split into packet types determined by disciminators */

/*! GEN_DWT_EVENT_CNT() - generate the DWT event count packet - flag bits indicate event
 */
#define GEN_DWT_EVENT_CNT(flags) DWT_HEADER(0, ITM_SWIT_SZ_8_BIT), (VAL_8_BIT(flags) & 0x3F),


#define GEN_DWT_EXCEP_TRC(excep_num, excep_fn) DWT_HEADER(1, ITM_SWIT_SZ_16_BIT), VAL_8_BIT(excep_num), \
											  ((VAL_8_BIT(excep_num >> 8) & 0x1) | (((uint8_t)(excep_fn) & 0x3) << 4)),

#define GEN_DWT_PC_SAMPLE(pc_val)	DWT_HEADER(2, ITM_SWIT_SZ_32_BIT), VAL_32_BIT(pc_val),
#define GEN_DWT_PC_SLEEP()			DWT_HEADER(2, ITM_SWIT_SZ_8_BIT), VAL_8_BIT(0),

#define GEN_DWT_DT_PC_VAL(cmpn, pc_val) DWT_HEADER(DWT_DT_DISC_ID_PC(cmpn), ITM_SWIT_SZ_32_BIT), VAL_32_BIT(pc_val),
#define GEN_DWT_DT_ADDR(cmpn, addr_val) DWT_HEADER(DWT_DT_DISC_ID_ADDR(cmpn), ITM_SWIT_SZ_16_BIT), VAL_16_BIT(addr_val),

#define GEN_DWT_DT_DATA_R_8(cmpn, data_val) DWT_HEADER(DWT_DT_DISC_ID_DATA_R(cmpn), ITM_SWIT_SZ_8_BIT), VAL_8_BIT(data_val),
#define GEN_DWT_DT_DATA_R_16(cmpn, data_val) DWT_HEADER(DWT_DT_DISC_ID_DATA_R(cmpn), ITM_SWIT_SZ_16_BIT), VAL_16_BIT(data_val),
#define GEN_DWT_DT_DATA_R_32(cmpn, data_val) DWT_HEADER(DWT_DT_DISC_ID_DATA_R(cmpn), ITM_SWIT_SZ_32_BIT), VAL_32_BIT(data_val),

#define GEN_DWT_DT_DATA_W_8(cmpn, data_val) DWT_HEADER(DWT_DT_DISC_ID_DATA_W(cmpn), ITM_SWIT_SZ_8_BIT), VAL_8_BIT(data_val),
#define GEN_DWT_DT_DATA_W_16(cmpn, data_val) DWT_HEADER(DWT_DT_DISC_ID_DATA_W(cmpn), ITM_SWIT_SZ_16_BIT), VAL_16_BIT(data_val),
#define GEN_DWT_DT_DATA_W_32(cmpn, data_val) DWT_HEADER(DWT_DT_DISC_ID_DATA_W(cmpn), ITM_SWIT_SZ_32_BIT), VAL_32_BIT(data_val),



/*! GEN_LTS_ (): generators for Local TS packets
 */
#define GEN_LTS_HDR_2_0_BITS_SYNC(val) TS_HDR_TS(val),

#define GEN_LTS_6_0_BITS(val, tc) TS_HDR_TC(tc), TS_7_BITS(val),

#define GEN_LTS_13_0_BITS(val, tc) TS_HDR_TC(tc), TS_7_BITS_CONT(val), TS_7_BITS(val >> 7),
 
#define GEN_LTS_20_0_BITS(val, tc) TS_HDR_TC(tc), TS_7_BITS_CONT(val), \
								 TS_7_BITS_CONT(val >> 7), TS_7_BITS(val >> 14),

#define GEN_LTS_27_0_BITS(val, tc) TS_HDR_TC(tc), TS_7_BITS_CONT(val), \
								 TS_7_BITS_CONT(val >> 7), TS_7_BITS_CONT(val >> 14), \
								 TS_7_BITS(val >> 21), 


#define GEN_GTS1_6_0_BITS(time)  GTS1_HEADER(), TS_7_BITS(time),

#define GEN_GTS1_13_0_BITS(time) GTS1_HEADER(), TS_7_BITS_CONT(time), TS_7_BITS(time >> 7),

#define GEN_GTS1_20_0_BITS(time) GTS1_HEADER(), TS_7_BITS_CONT(time), \
												TS_7_BITS_CONT(time >> 7), TS_7_BITS(time >> 14),

#define GEN_GTS1_25_0_BITS_FLAGS(time, wrap, clk_change) \
								 GTS1_HEADER(),	TS_7_BITS_CONT(time), \
												TS_7_BITS_CONT(time >> 7), TS_7_BITS_CONT(time >> 14), \
												GTS1_BYTE3(time, wrap, clk_change),

#define GEN_GTS1_25_0_BITS(time) GEN_GTS1_25_0_BITS_FLAGS(time, 0, 0)

#define GEN_GTS2_48_BIT(time) GTS2_HEADER(), TS_7_BITS_CONT(time >> 26), TS_7_BITS_CONT(time >> 33), \
									         TS_7_BITS_CONT(time >> 40), (TS_7_BITS(time >> 47) & 0x1),

#define GEN_GTS2_64_BIT(time) GTS2_HEADER(), TS_7_BITS_CONT(time >> 26), TS_7_BITS_CONT(time >> 33), \
											 TS_7_BITS_CONT(time >> 40), TS_7_BITS_CONT(time >> 47), \
											 TS_7_BITS_CONT(time >> 54), (TS_7_BITS(time >> 61) & 0x7),


#define GEN_EXT_SWIT_PAGE(page) ((((uint8_t)(page) & 0x7) << 4) | 0x8),

#endif // ARM_ITM_TEST_GEN_H_INCLUDED


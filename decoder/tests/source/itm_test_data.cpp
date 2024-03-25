/*!
 * \file       itm_test_data.cpp
 * \brief      OpenCSD : Test data for ITM decoder test
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

#include "itm_test.h"
#include "itm_test_gen.h"


/* test data arrays use packet generator macros that resolve to byte arrays with appropriate bits shifted and set
   each generator includes the trailing ',' to allow concatenation with next generator.
*/

/* arbitrary unsynced data before sync. some 0x00 bytes to test found / not found async */
static const char* test_0_desc = "Test arbitrary data before ASYNC, short sequence after";
static const uint8_t test_0_data[] = {
	0xF0, 0x00, 0x00, 0x34,
	0x00, 0x12, 0x33, 0x44,
	0x12, 0x43, 0x55, 0x66,
	0x22, 0x77, 0x88, 0x99,
	GEN_ASYNC()
	GEN_OVERFLOW()
	GEN_SWIT_1_BYTE(3, 0xBB)
	GEN_LTS_HDR_2_0_BITS_SYNC(2)
};

/* test of various SWIT and local TS packets */
static const char* test_1_desc = "test of various SWIT and local TS packets";
static const uint8_t test_1_data[] = {
	GEN_ASYNC()
	GEN_SWIT_1_BYTE(1, 0xAC)
	GEN_LTS_HDR_2_0_BITS_SYNC(2)
	GEN_SWIT_2_BYTE(1, 0x2345)
	GEN_SWIT_4_BYTE(1, 0x67890123)
	GEN_LTS_6_0_BITS(13, ITM_LTS_TC_TS_DELAY_FLAG)
};

/* test DWT packets and LTS types */
static const char* test_2_desc = "test DWT packets and LTS types.";
static const uint8_t test_2_data[] = {
	GEN_ASYNC()
	GEN_DWT_EVENT_CNT(0x15)
	GEN_LTS_20_0_BITS(0x12402, ITM_LTS_TC_TS_SYNC_FLAG)
	GEN_DWT_EXCEP_TRC(4,  DWT_EXCEP_FN_ENTER)
	GEN_LTS_13_0_BITS(0x3220, ITM_LTS_TC_TS_DELAY_FLAG)
	GEN_DWT_PC_SAMPLE(0x1000)
	GEN_LTS_13_0_BITS(0x1342, ITM_LTS_TC_PKT_DELAY_FLAG)
	GEN_DWT_PC_SAMPLE(0x1080)
	GEN_DWT_PC_SLEEP()
	GEN_OVERFLOW()
	GEN_DWT_DT_DATA_R_8(ITM_DWT_CMPN_1, 0x44)
	GEN_LTS_27_0_BITS(0x7543F5e, ITM_LTS_TC_TS_AND_PKT_DELAY_FLAG)
	GEN_DWT_DT_DATA_R_16(ITM_DWT_CMPN_0, 0x5566)
	GEN_LTS_6_0_BITS(0x78,  ITM_LTS_TC_TS_SYNC_FLAG)
	GEN_DWT_DT_DATA_R_32(ITM_DWT_CMPN_2, 0x778899AA)
	GEN_LTS_HDR_2_0_BITS_SYNC(6)
	GEN_DWT_EXCEP_TRC(0x123, DWT_EXCEP_FN_EXIT)
	GEN_LTS_HDR_2_0_BITS_SYNC(3)
	GEN_DWT_EXCEP_TRC(0x3, DWT_EXCEP_FN_RETURN)
	GEN_DWT_DT_PC_VAL(ITM_DWT_CMPN_3, 0x2000)
	GEN_DWT_DT_ADDR(ITM_DWT_CMPN_0, 0x12340322)
	GEN_DWT_DT_DATA_W_8(ITM_DWT_CMPN_0, 0x11)
	GEN_LTS_HDR_2_0_BITS_SYNC(4)
	GEN_DWT_DT_DATA_W_16(ITM_DWT_CMPN_1, 0x2233)
	GEN_LTS_HDR_2_0_BITS_SYNC(3)
	GEN_DWT_DT_DATA_W_32(ITM_DWT_CMPN_2, 0x44556677)
	GEN_LTS_HDR_2_0_BITS_SYNC(2)
};

/* test SWIT with channel extension */
static const char* test_3_desc = "test SWIT with channel extension";
static const uint8_t test_3_data[] = {
	GEN_ASYNC()
	GEN_OVERFLOW()
	GEN_SWIT_1_BYTE(0xA, 0xAC)
	GEN_LTS_HDR_2_0_BITS_SYNC(2)
	GEN_EXT_SWIT_PAGE(2)
	GEN_SWIT_2_BYTE(0xA, 0x2345)
	GEN_EXT_SWIT_PAGE(4)
	GEN_SWIT_4_BYTE(0xB, 0x67890123)
	GEN_EXT_SWIT_PAGE(0)
	GEN_SWIT_2_BYTE(0xA, 0x32fe)
};

/* test GTS clock packets. here the GEN_OVERFLOW() will zero out the 64 timestamp that is used for
 * accumlation the LTS deltas - in case we are running the tests in series!
 * Also test that after wrap we never output a GTS until we see a new GTS2 packet
 * We mix 48 and 64 bit GTS 2 packets here - which would not happen in reality - but is fine to test the decoder. 
 */
static const char* test_4_desc = "test GTS clock packets in SWIT sequence";
static const uint8_t test_4_data[] = {
	GEN_ASYNC()
	GEN_OVERFLOW()
	GEN_GTS1_25_0_BITS(0xF23456)
	GEN_GTS2_64_BIT(0x1020304C000000ULL)
	GEN_SWIT_2_BYTE(0x10, 0x1234)
	GEN_GTS1_6_0_BITS(0x7A)
	GEN_SWIT_4_BYTE(0x10, 0x56789abc)
	GEN_GTS1_13_0_BITS(0x16712)
	GEN_SWIT_2_BYTE(0x11, 0x9876)
	GEN_GTS1_20_0_BITS(0x42353)
	GEN_SWIT_4_BYTE(0x11, 0x54321adc)
	GEN_GTS1_25_0_BITS(0x2FEA782)
	GEN_SWIT_2_BYTE(0x11, 0xa5a5)
	GEN_GTS1_25_0_BITS_FLAGS(0x2FEA782, 1 , 1)
	GEN_SWIT_2_BYTE(0x11, 0xb6b6)
	GEN_GTS2_64_BIT(0x1020304F000000ULL)
	GEN_SWIT_2_BYTE(0x11, 0xc7c7)
	GEN_GTS1_25_0_BITS_FLAGS(0x343923a, 1 , 0)
	GEN_SWIT_2_BYTE(0x11, 0xc7c7)
	GEN_GTS1_6_0_BITS(0xdd)
	GEN_SWIT_2_BYTE(0x11, 0xd8d8)
	GEN_GTS2_64_BIT(0x10203050000000ULL)
	GEN_SWIT_2_BYTE(0x11, 0xe9e9)
	GEN_GTS1_13_0_BITS(0x3451)
	GEN_SWIT_2_BYTE(0x10, 0xf0f0)
	GEN_GTS1_25_0_BITS_FLAGS(0x03afe32, 1 , 0)
	GEN_GTS2_48_BIT(0x123458000000ULL)
	GEN_SWIT_2_BYTE(0x10, 0x0101)
};

/* set of tests */
static itm_test_data_t itm_tests_array[] =
{
	/* tests */
	{ "test0", test_0_desc, sizeof(test_0_data), test_0_data,} ,
	{ "test1", test_1_desc, sizeof(test_1_data), test_1_data,} ,
	{ "test2", test_2_desc, sizeof(test_2_data), test_2_data,} ,
	{ "test3", test_3_desc, sizeof(test_3_data), test_3_data,} ,
	{ "test4", test_4_desc, sizeof(test_4_data), test_4_data,} ,

	/* end of tests marker */
	{0},
};

itm_test_data_t* get_item_tests()
{
	return itm_tests_array;
}

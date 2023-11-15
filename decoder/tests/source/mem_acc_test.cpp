/*
 * \file       mem_acc_test.cpp
 * \brief      OpenCSD : Component tests for memory accessor and caching
 *
 * \copyright  Copyright (c) 2023, ARM Limited. All Rights Reserved.
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

/* 
 * Test program to validate the memory accessor and caching.
 * 
 * Checks for memory spaces, overlapping VA, cache sizes 
 */

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>

#include "opencsd.h"  

// logging
static ocsdMsgLogger logger;
static ocsdDefaultErrorLogger err_log;
static ocsd_hndl_err_log_t err_log_handle;
static int logOpts = ocsdMsgLogger::OUT_STDOUT | ocsdMsgLogger::OUT_FILE;
static std::string logfileName = "mem_acc_test.ppl";

// test pass fail counts
static int tests_passed = 0;
static int tests_failed = 0;

// number of blocks per memory area
#define NUM_BLOCKS 2
// number of 32 but words in each block
#define BLOCK_NUM_WORDS 8192
// size of block in bytes
#define BLOCK_SIZE_BYTES (4 * BLOCK_NUM_WORDS)

// some memory areas to use for testing
static uint32_t el01_ns_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el2_ns_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el01_s_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el2_s_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el3_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el01_r_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el2_r_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];
static uint32_t el3_root_blocks[NUM_BLOCKS][BLOCK_NUM_WORDS];

#define BLOCK_VAL(mem_space, block_num, index) (uint32_t)(((uint32_t)mem_space << 24) | ((uint32_t)block_num << 16) | (uint32_t)index)

// memory access mapper - used for tests
static TrcMemAccMapGlobalSpace mapper;



void populate_block(const ocsd_mem_space_acc_t mem_space, uint32_t block_array[NUM_BLOCKS][BLOCK_NUM_WORDS])
{
    for (int i = 0; i < NUM_BLOCKS; i++) {
        for (int j = 0; j < BLOCK_NUM_WORDS; j++)
            block_array[i][j] = BLOCK_VAL(mem_space, i, j);
    }
}

void populate_all_blocks()
{
    populate_block(OCSD_MEM_SPACE_EL1N, el01_ns_blocks);
    populate_block(OCSD_MEM_SPACE_EL2, el2_ns_blocks);
    populate_block(OCSD_MEM_SPACE_EL1S, el01_s_blocks);
    populate_block(OCSD_MEM_SPACE_EL2S, el2_s_blocks);
    populate_block(OCSD_MEM_SPACE_EL3, el3_blocks);
    populate_block(OCSD_MEM_SPACE_EL1R, el01_r_blocks);
    populate_block(OCSD_MEM_SPACE_EL2R, el2_r_blocks);
    populate_block(OCSD_MEM_SPACE_ROOT, el3_root_blocks);
}

bool process_cmd_line_logger_opts(int argc, char* argv[])
{
    bool goodLoggerOpts = true;
    bool bChangingOptFlags = false;
    int newlogOpts = ocsdMsgLogger::OUT_NONE;
    std::string opt;
    if (argc > 1)
    {
        int options_to_process = argc - 1;
        int optIdx = 1;
        while (options_to_process > 0)
        {
            opt = argv[optIdx];
            if (opt == "-logstdout")
            {
                newlogOpts |= ocsdMsgLogger::OUT_STDOUT;
                bChangingOptFlags = true;
            }
            else if (opt == "-logstderr")
            {
                newlogOpts |= ocsdMsgLogger::OUT_STDERR;
                bChangingOptFlags = true;
            }
            else if (opt == "-logfile")
            {
                newlogOpts |= ocsdMsgLogger::OUT_FILE;
                bChangingOptFlags = true;
            }
            else if (opt == "-logfilename")
            {
                options_to_process--;
                optIdx++;
                if (options_to_process)
                {
                    logfileName = argv[optIdx];
                    newlogOpts |= ocsdMsgLogger::OUT_FILE;
                    bChangingOptFlags = true;
                }
                else
                {
                    goodLoggerOpts = false;
                }
            }
            options_to_process--;
            optIdx++;
        }
    }
    if (bChangingOptFlags)
        logOpts = newlogOpts;
    return goodLoggerOpts;
}

void log_error(const ocsdError& err)
{
    err_log.LogError(err_log_handle, &err);
}

void log_test_start(const char* testname)
{
    std::ostringstream oss;
    oss << "*** Test " << testname << " Starting.\n";
    logger.LogMsg(oss.str());
}

void log_test_end(const char* testname, const int pass, const int fail)
{
    std::ostringstream oss;
    oss << "*** Test " << testname << " complete. (Pass: " << pass << "; Fail:" << fail << ")\n";
    logger.LogMsg(oss.str());
}

void test_overlap_regions()
{
    // test adding regions that overlap
    // overlap in difference memory spaces is ok, otherwise fail
    TrcMemAccBufPtr Acc1, Acc2, Acc3, Acc4;
    ocsd_err_t err;
    std::ostringstream oss;
    int passed = 0, failed = 0;

    log_test_start(__FUNCTION__);

    // add single accessor
    Acc1.initAccessor(0x0000, (const uint8_t*)&el01_ns_blocks[0], BLOCK_SIZE_BYTES);
    Acc1.setMemSpace(OCSD_MEM_SPACE_EL1N);
    err = mapper.AddAccessor(&Acc1, 0);
    if (err != OCSD_OK) {
        log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set memory accessor"));
        failed++;
    }
    else
        passed++;

    // overlapping region - same memory space.
    Acc2.initAccessor(0x1000, (const uint8_t*)&el01_ns_blocks[1], BLOCK_SIZE_BYTES);
    Acc2.setMemSpace(OCSD_MEM_SPACE_EL1N);
    err = mapper.AddAccessor(&Acc2, 0);
    if (err != OCSD_ERR_MEM_ACC_OVERLAP) {
        oss.str("");
        oss << "Error: expected OCSD_ERR_MEM_ACC_OVERLAP error for overlapping accessor range.\n";
        logger.LogMsg(oss.str());
        failed++;
    }
    else
        passed++;

    // non overlapping region - same memory space.
    Acc2.setRange(0x8000, 0x8000 + BLOCK_SIZE_BYTES - 1);
    err = mapper.AddAccessor(&Acc2, 0);
    if (err != OCSD_OK) {
        log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set non overlapping memory accessor"));
        failed++;
    }
    else
        passed++;

    // overlapping region - different memory space
    Acc3.initAccessor(0x0000, (const uint8_t*)&el01_s_blocks[0], BLOCK_SIZE_BYTES);
    Acc3.setMemSpace(OCSD_MEM_SPACE_EL1S);
    err = mapper.AddAccessor(&Acc3, 0);
    if (err != OCSD_OK) {
        log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set overlapping memory accessor in other memory space"));
        failed++;
    }
    else
        passed++;

    // overlapping region - more general memory space.
    Acc4.initAccessor(0x0000, (const uint8_t*)&el2_s_blocks[0], BLOCK_SIZE_BYTES);
    Acc4.setMemSpace(OCSD_MEM_SPACE_S);
    err = mapper.AddAccessor(&Acc4, 0);
    if (err != OCSD_ERR_MEM_ACC_OVERLAP) {
        oss.str("");
        oss << "Error: expected OCSD_ERR_MEM_ACC_OVERLAP error for overlapping general _S accessor range.\n";
        logger.LogMsg(oss.str());
        failed++;
    }
    else
        passed++;

    // clean up mapper
    mapper.RemoveAllAccessors();
    tests_passed += passed;
    tests_failed += failed;

    log_test_end(__FUNCTION__, passed, failed);
}
/************************************************************************
 * Test trcID specific memory regions - using callback function.         
 * Emulates clinets such as perf where memory regions change over the 
 * trace run as tasks switched in and out. Tests caching mechanisms in 
 * mapper.
 */
typedef struct test_range {
    ocsd_vaddr_t s_address;
    uint32_t size;
    const uint8_t* buffer; 
    ocsd_mem_space_acc_t mem_space;
    uint8_t trcID;
} test_range_t;

typedef struct test_range_array {
    int num_ranges;
    test_range_t* ranges;
} test_range_array_t;

#define IN_MEM_SPACE(m1, m2) (bool)(((uint32_t)m1)&((uint32_t)m2))

static int AccCallbackCount = 0;

uint32_t TestMemAccCB(const void* p_context, const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t trcID, const uint32_t reqBytes, uint8_t* byteBuffer)
{
    test_range_array_t* ranges = (test_range_array_t*)p_context;
    uint32_t bytes_read = 0;

    for (int i = 0; i < ranges->num_ranges; i++)
    {
        if ( IN_MEM_SPACE(mem_space, ranges->ranges[i].mem_space) && 
             (trcID == ranges->ranges[i].trcID)
            ) 
        {
            if ( (address >= ranges->ranges[i].s_address) && 
                 (address < (ranges->ranges[i].s_address + ranges->ranges[i].size))
               )
            {
                // in range - get offset into buffer
                uint32_t offset =  address - ranges->ranges[i].s_address;

                // copy all bytes if enough left in range - otherwise what if left
                if (ranges->ranges[i].size - offset >= reqBytes)
                    bytes_read = reqBytes;
                else
                    bytes_read = ranges->ranges[i].size - offset;

                memcpy(byteBuffer, &(ranges->ranges[i].buffer[offset]), bytes_read);
                break;
            }
        }
    }
    AccCallbackCount++;
    return bytes_read;
}

void set_test_range(test_range_t& range, ocsd_vaddr_t s_address, uint32_t size,
                    const uint8_t* buffer, ocsd_mem_space_acc_t mem_space, uint8_t trcID)
{
    range.s_address = s_address;
    range.size = size;
    range.buffer = buffer;
    range.mem_space = mem_space;
    range.trcID = trcID;
}

// read value through mapper and direct from range - check value and callback happened
// allows testing of expected caching and callback events 
bool read_and_check_from_range(const int test_idx, const int range, test_range_array_t& ranges, const ocsd_vaddr_t byte_offset, bool callback)
{
    uint32_t read_val, num_bytes_read, expected_val;
    uint8_t* p_local_buff = (uint8_t*)&read_val;
    int PrevAccCallbackCount = AccCallbackCount;
    ocsd_err_t err;
    ocsd_vaddr_t read_address;
    bool pass = true, mem_callback_occurred = false;
    std::ostringstream oss;
    std::string memSpaceStr;
    ocsd_mem_space_acc_t mem_space;
    uint8_t traceID;


    mem_space = ranges.ranges[range].mem_space;
    read_address = ranges.ranges[range].s_address + byte_offset;  // set address value
    num_bytes_read = 4; // request 4 bytes
    expected_val = *((uint32_t *)(&(ranges.ranges[range].buffer[byte_offset])));
    traceID = ranges.ranges[range].trcID;
    TrcMemAccessorBase::getMemAccSpaceString(memSpaceStr, mem_space);

    oss << "Read Test(" << test_idx << "): Address 0x" << std::hex << std::setw(8) << std::setfill('0') << read_address << "; ";
    oss << memSpaceStr << "; Traced ID 0x" << std::setw(2) << (uint32_t)traceID << "; ";
    logger.LogMsg(oss.str());

    err = mapper.ReadTargetMemory(read_address, ranges.ranges[range].trcID, OCSD_MEM_SPACE_EL1N, &num_bytes_read, p_local_buff);

    mem_callback_occurred = (bool)(PrevAccCallbackCount != AccCallbackCount);
    if (mem_callback_occurred)
        oss.str("MemCB read; ");
    else
        oss.str("No MemCB; ");
    logger.LogMsg(oss.str());
    
    if (err != OCSD_OK) {
        oss.str("");
        oss << "Error reading target memory\n";
        logger.LogMsg(oss.str());
        log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, oss.str()));
        pass = false;
        goto exit_test;
    }

    if (num_bytes_read != 4) {
        oss.str("");
        oss << "Read Fail: Requested bytes not found (4 != " << num_bytes_read << ")\n";
        logger.LogMsg(oss.str());
        pass = false;
        goto exit_test;
    }

    if (expected_val != read_val) {
        oss.str("");
        oss << "Read Fail: value read mismatch; 0x" << std::hex << read_val << " != 0x" << expected_val << "\n";
        logger.LogMsg(oss.str());
        pass = false;
        goto exit_test;
    }

    if (callback && !mem_callback_occurred) {
        oss.str("");
        oss << "Read Fail: Expected callback to access memory\n";
        logger.LogMsg(oss.str());
        pass = false;
        goto exit_test;
    }

    if (!callback && mem_callback_occurred) {
        oss.str("");
        oss << "Read Fail: Unexpected callback to access memory\n";
        logger.LogMsg(oss.str());
        pass = false;
        goto exit_test;
    }

    if (pass) {
        oss.str("\n");
        logger.LogMsg(oss.str());
    }

exit_test:
    return pass;
}

void test_trcid_cache_mem_cb()
{
    TrcMemAccCB CBAcc;
    test_range_array_t ranges;
    int passed = 0, failed = 0;
    std::ostringstream oss;
    ocsd_err_t err;
    int read_test_idx = 1;
    

    log_test_start(__FUNCTION__);

    // set up the ranges
    ranges.num_ranges = 6;
    ranges.ranges = new test_range_t[6];
    
    // 1st range 0x0000, EL1N, trcID 0x10 - 1st CPU
    set_test_range(ranges.ranges[0], 0x0000, BLOCK_SIZE_BYTES, (const uint8_t*)&el01_ns_blocks[0], OCSD_MEM_SPACE_EL1N, 0x10);
    // 2nd range 0x0000, EL1N, trcID 0x11 - 2nd cpu, same addresses, different data. 
    set_test_range(ranges.ranges[1], 0x0000, BLOCK_SIZE_BYTES, (const uint8_t*)&el01_ns_blocks[1], OCSD_MEM_SPACE_EL1N, 0x11);
    // 2nd range 0x8000, EL2N, trcID 0x10 - 2nd cpu, same addresses, different data. 
    set_test_range(ranges.ranges[2], 0x8000, BLOCK_SIZE_BYTES, (const uint8_t*)&el2_ns_blocks[0], OCSD_MEM_SPACE_EL2, 0x10);
    // 2nd range 0x10000, EL2N, trcID 0x11 - 2nd cpu, 
    set_test_range(ranges.ranges[3], 0x10000, BLOCK_SIZE_BYTES, (const uint8_t*)&el2_ns_blocks[1], OCSD_MEM_SPACE_EL2, 0x11);
    // 2nd range 0x0000, EL1 realm, trcID 0x10 - cpu 1 realm
    set_test_range(ranges.ranges[4], 0x0000, BLOCK_SIZE_BYTES, (const uint8_t*)&el01_r_blocks[0], OCSD_MEM_SPACE_EL1R, 0x10);
    // 2nd range 0x0000, EL2 realm, trcID 0x11 - 2nd cpu - realm . 
    set_test_range(ranges.ranges[5], 0x0000, BLOCK_SIZE_BYTES, (const uint8_t*)&el2_r_blocks[0], OCSD_MEM_SPACE_EL2R, 0x11);

    // add the callback to the mapper
    CBAcc.initAccessor(0, 0xFFFFFFFF, OCSD_MEM_SPACE_ANY);
    CBAcc.setCBIDIfFn(TestMemAccCB, (void*)&ranges);
    err = mapper.AddAccessor(&CBAcc, 0);
    if (err != OCSD_OK) {
        log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set callback memory accessor"));
        failed++;
        goto cleanup;
    }

    // run some tests 
    // initial read - should callback and load cache page
    read_and_check_from_range(read_test_idx++, 0, ranges, 0, true) ? passed++ : failed++; 

    // next read - should not callback but use cache
    read_and_check_from_range(read_test_idx++, 0, ranges, 0x10, false) ? passed++ : failed++; 

    // different cpu - same address - should callback for cache load
    read_and_check_from_range(read_test_idx++, 1, ranges, 0x10, true) ? passed++ : failed++;

    // different cpu - same address - use cache
    read_and_check_from_range(read_test_idx++, 1, ranges, 0x10, false) ? passed++ : failed++;


    // clean up mapper
cleanup:
    mapper.RemoveAllAccessors();
    tests_passed += passed;
    tests_failed += failed;
    delete[] ranges.ranges;

    log_test_end(__FUNCTION__, passed, failed);
}

/************************************************************************
 * Test trcID specific memory regions - using callback function.
 * Emulates clinets such as perf where memory regions change over the
 * trace run as tasks switched in and out. Tests caching mechanisms in
 * mapper.
 */

#define TEST_ADDR_COMMON 0x000000
#define TEST_ADDR_EL1N   0x008000
#define TEST_ADDR_EL2    0x010000
#define TEST_ADDR_EL1S   0x018000
#define TEST_ADDR_EL2S   0x020000
#define TEST_ADDR_EL3    0x028000
#define TEST_ADDR_EL1R   0x030000
#define TEST_ADDR_EL2R   0x038000
#define TEST_ADDR_EL3R   0x040000

bool read_and_check_value(ocsd_vaddr_t addr, const uint8_t* p_block_buffer, ocsd_mem_space_acc_t space)
{
    ocsd_err_t err;
    uint32_t read_val, check_val, num_bytes;
    std::ostringstream oss;
    std::string memSpaceStr;


    TrcMemAccessorBase::getMemAccSpaceString(memSpaceStr, space);
    oss << "Read Test: Address 0x" << std::hex << std::setw(8) << std::setfill('0') << addr << "; ";
    oss << std::setw(4) << std::setfill(' ') << memSpaceStr << ";" ;
    logger.LogMsg(oss.str());


    num_bytes = 4;
    err = mapper.ReadTargetMemory(addr, 0, space, &num_bytes, (uint8_t*)&read_val);
    if (err != OCSD_OK) {
        log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to read from mapper"));
        return false;
    }

    check_val = *((uint32_t *)p_block_buffer);
    if (check_val != read_val)
    {
        oss.str("");
        oss << "Read Fail: value read mismatch; 0x" << std::hex << read_val << " != 0x" << check_val << "\n";
        logger.LogMsg(oss.str());
        return false;
    }

    oss.str("");
    oss << " [0x" << std::hex << std::setw(8) << std::setfill('0') << read_val << "]\n";
    logger.LogMsg(oss.str());
    return true;
}

void test_mem_spaces()
{
    ocsd_err_t err;
    int passed = 0, failed = 0;
    std::ostringstream oss;


    log_test_start(__FUNCTION__);

    // check each memory space, 8 of them, and a each block.
    #define NUM_ACCS (NUM_BLOCKS * 8)
    TrcMemAccBufPtr accs[NUM_ACCS];

    //  set-up accessors - block 0 in each space is the same - the next will have no overlap
    accs[0].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el01_ns_blocks[0], BLOCK_SIZE_BYTES);
    accs[0].setMemSpace(OCSD_MEM_SPACE_EL1N);
    accs[1].initAccessor(TEST_ADDR_EL1N, (const uint8_t*)&el01_ns_blocks[1], BLOCK_SIZE_BYTES);
    accs[1].setMemSpace(OCSD_MEM_SPACE_EL1N);

    accs[2].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el2_ns_blocks[0], BLOCK_SIZE_BYTES);
    accs[2].setMemSpace(OCSD_MEM_SPACE_EL2);
    accs[3].initAccessor(TEST_ADDR_EL2, (const uint8_t*)&el2_ns_blocks[1], BLOCK_SIZE_BYTES);
    accs[3].setMemSpace(OCSD_MEM_SPACE_EL2);

    accs[4].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el01_s_blocks[0], BLOCK_SIZE_BYTES);
    accs[4].setMemSpace(OCSD_MEM_SPACE_EL1S);
    accs[5].initAccessor(TEST_ADDR_EL1S, (const uint8_t*)&el01_s_blocks[1], BLOCK_SIZE_BYTES);
    accs[5].setMemSpace(OCSD_MEM_SPACE_EL1S);

    accs[6].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el2_s_blocks[0], BLOCK_SIZE_BYTES);
    accs[6].setMemSpace(OCSD_MEM_SPACE_EL2S);
    accs[7].initAccessor(TEST_ADDR_EL2S, (const uint8_t*)&el2_s_blocks[1], BLOCK_SIZE_BYTES);
    accs[7].setMemSpace(OCSD_MEM_SPACE_EL2S);

    accs[8].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el3_blocks[0], BLOCK_SIZE_BYTES);
    accs[8].setMemSpace(OCSD_MEM_SPACE_EL3);
    accs[9].initAccessor(TEST_ADDR_EL3, (const uint8_t*)&el3_blocks[1], BLOCK_SIZE_BYTES);
    accs[9].setMemSpace(OCSD_MEM_SPACE_EL3);

    accs[10].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el01_r_blocks[0], BLOCK_SIZE_BYTES);
    accs[10].setMemSpace(OCSD_MEM_SPACE_EL1R);
    accs[11].initAccessor(TEST_ADDR_EL1R, (const uint8_t*)&el01_r_blocks[1], BLOCK_SIZE_BYTES);
    accs[11].setMemSpace(OCSD_MEM_SPACE_EL1R);

    accs[12].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el2_r_blocks[0], BLOCK_SIZE_BYTES);
    accs[12].setMemSpace(OCSD_MEM_SPACE_EL2R);
    accs[13].initAccessor(TEST_ADDR_EL2R, (const uint8_t*)&el2_r_blocks[1], BLOCK_SIZE_BYTES);
    accs[13].setMemSpace(OCSD_MEM_SPACE_EL2R);

    accs[14].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el3_root_blocks[0], BLOCK_SIZE_BYTES);
    accs[14].setMemSpace(OCSD_MEM_SPACE_ROOT);
    accs[15].initAccessor(TEST_ADDR_EL3R, (const uint8_t*)&el3_root_blocks[1], BLOCK_SIZE_BYTES);
    accs[15].setMemSpace(OCSD_MEM_SPACE_ROOT);

    // add accessors to mapper.
    for (int i = 0; i < NUM_ACCS; i++) {
        err = mapper.AddAccessor(&accs[i], 0);
        if (err != OCSD_OK) {
            log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set callback memory accessor"));
            failed++;
        }
        else
            passed++;
    }

    // test a bunch of reads - use specific space then check match if more global spaces are used.
    oss.str("Test EL1N registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el01_ns_blocks[0], OCSD_MEM_SPACE_EL1N) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1N, (const uint8_t*)&el01_ns_blocks[1], OCSD_MEM_SPACE_EL1N) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1N, (const uint8_t*)&el01_ns_blocks[1], OCSD_MEM_SPACE_N) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1N, (const uint8_t*)&el01_ns_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test EL1N registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el2_ns_blocks[0], OCSD_MEM_SPACE_EL2) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2, (const uint8_t*)&el2_ns_blocks[1], OCSD_MEM_SPACE_EL2) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2, (const uint8_t*)&el2_ns_blocks[1], OCSD_MEM_SPACE_N) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2, (const uint8_t*)&el2_ns_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test EL1S registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el01_s_blocks[0], OCSD_MEM_SPACE_EL1S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1S, (const uint8_t*)&el01_s_blocks[1], OCSD_MEM_SPACE_EL1S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1S, (const uint8_t*)&el01_s_blocks[1], OCSD_MEM_SPACE_S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1S, (const uint8_t*)&el01_s_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test EL2S registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el2_s_blocks[0], OCSD_MEM_SPACE_EL2S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2S, (const uint8_t*)&el2_s_blocks[1], OCSD_MEM_SPACE_EL2S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2S, (const uint8_t*)&el2_s_blocks[1], OCSD_MEM_SPACE_S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2S, (const uint8_t*)&el2_s_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test EL3 registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el3_blocks[0], OCSD_MEM_SPACE_EL3) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL3, (const uint8_t*)&el3_blocks[1], OCSD_MEM_SPACE_EL3) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL3, (const uint8_t*)&el3_blocks[1], OCSD_MEM_SPACE_S) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL3, (const uint8_t*)&el3_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test EL1R registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el01_r_blocks[0], OCSD_MEM_SPACE_EL1R) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1R, (const uint8_t*)&el01_r_blocks[1], OCSD_MEM_SPACE_EL1R) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1R, (const uint8_t*)&el01_r_blocks[1], OCSD_MEM_SPACE_R) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL1R, (const uint8_t*)&el01_r_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test EL2R registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el2_r_blocks[0], OCSD_MEM_SPACE_EL2R) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2R, (const uint8_t*)&el2_r_blocks[1], OCSD_MEM_SPACE_EL2R) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2R, (const uint8_t*)&el2_r_blocks[1], OCSD_MEM_SPACE_R) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL2R, (const uint8_t*)&el2_r_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    oss.str("Test ROOT registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON, (const uint8_t*)&el3_root_blocks[0], OCSD_MEM_SPACE_ROOT) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL3R, (const uint8_t*)&el3_root_blocks[1], OCSD_MEM_SPACE_ROOT) ? passed++ : failed++;
    read_and_check_value(TEST_ADDR_EL3R, (const uint8_t*)&el3_root_blocks[1], OCSD_MEM_SPACE_ANY) ? passed++ : failed++;

    // clear for this test 
    mapper.RemoveAllAccessors();

    // test access more global from specific - set ANY, N S and R spaces
    accs[0].initAccessor(TEST_ADDR_COMMON, (const uint8_t*)&el01_ns_blocks[0], BLOCK_SIZE_BYTES);
    accs[0].setMemSpace(OCSD_MEM_SPACE_ANY);
    accs[1].initAccessor(TEST_ADDR_EL1N, (const uint8_t*)&el01_ns_blocks[1], BLOCK_SIZE_BYTES);
    accs[1].setMemSpace(OCSD_MEM_SPACE_N);
    accs[2].initAccessor(TEST_ADDR_EL2, (const uint8_t*)&el2_ns_blocks[0], BLOCK_SIZE_BYTES);
    accs[2].setMemSpace(OCSD_MEM_SPACE_S);
    accs[3].initAccessor(TEST_ADDR_EL3, (const uint8_t*)&el2_ns_blocks[1], BLOCK_SIZE_BYTES);
    accs[3].setMemSpace(OCSD_MEM_SPACE_R);

    for (int i = 0; i < 4; i++) {
        err = mapper.AddAccessor(&accs[i], 0);
        if (err != OCSD_OK) {
            log_error(ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set callback memory accessor"));
            failed++;
        }
        else
            passed++;
    }

    // for ANY space should match all other spaces
    uint32_t offset = 0;
    oss.str("Test ANY registered block\n");
    logger.LogMsg(oss.str());
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) +offset, OCSD_MEM_SPACE_EL1N) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL2) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL1S) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL2S) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL3) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL1R) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL2R) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_S) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_N) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_R) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_COMMON + offset, ((const uint8_t*)&el01_ns_blocks[0]) + offset, OCSD_MEM_SPACE_ROOT) ? passed++ : failed++;

    oss.str("Test Any N registered block\n");
    logger.LogMsg(oss.str());
    offset = 0;
    read_and_check_value(TEST_ADDR_EL1N + offset, ((const uint8_t*)&el01_ns_blocks[1]) + offset, OCSD_MEM_SPACE_EL1N) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL1N + offset, ((const uint8_t*)&el01_ns_blocks[1]) + offset, OCSD_MEM_SPACE_EL2) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL1N + offset, ((const uint8_t*)&el01_ns_blocks[1]) + offset, OCSD_MEM_SPACE_N) ? passed++ : failed++;

    oss.str("Test Any S registered block\n");
    logger.LogMsg(oss.str());
    offset = 0;
    read_and_check_value(TEST_ADDR_EL2 + offset, ((const uint8_t*)&el2_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL1S) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL2 + offset, ((const uint8_t*)&el2_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL2S) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL2 + offset, ((const uint8_t*)&el2_ns_blocks[0]) + offset, OCSD_MEM_SPACE_EL3) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL2 + offset, ((const uint8_t*)&el2_ns_blocks[0]) + offset, OCSD_MEM_SPACE_S) ? passed++ : failed++;

    oss.str("Test Any R registered block\n");
    logger.LogMsg(oss.str());
    offset = 0;
    read_and_check_value(TEST_ADDR_EL3 + offset, ((const uint8_t*)&el2_ns_blocks[1]) + offset, OCSD_MEM_SPACE_EL1R) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL3 + offset, ((const uint8_t*)&el2_ns_blocks[1]) + offset, OCSD_MEM_SPACE_EL2R) ? passed++ : failed++;
    offset += 4;
    read_and_check_value(TEST_ADDR_EL3 + offset, ((const uint8_t*)&el2_ns_blocks[1]) + offset, OCSD_MEM_SPACE_R) ? passed++ : failed++;
    offset += 4;

    // clean up
    mapper.RemoveAllAccessors();
    tests_passed += passed;
    tests_failed += failed;
    log_test_end(__FUNCTION__, passed, failed);
 }

/************************************************************************
 * main program 
 */
int main(int argc, char* argv[])
{
	std::ostringstream oss;

    if (!process_cmd_line_logger_opts(argc, argv))
    {
        std::cout << "Bad logger command line options\nProgram Exiting\n";
        return -1;
    }

    // init the loggers.
    logger.setLogOpts(logOpts);
    logger.setLogFileName(logfileName.c_str());
    err_log.initErrorLogger(OCSD_ERR_SEV_INFO);
    err_log_handle = err_log.RegisterErrorSource("MEMACCTEST");
    err_log.setOutputLogger(&logger);

    oss << "OpenCSD memory access tests.\n";
    oss << "----------------------------\n\n";
    oss << "Library Version : " << ocsdVersion::vers_str() << "\n\n";
    logger.LogMsg(oss.str());

    // set up test data
    populate_all_blocks();

    // init the mapper
    mapper.setErrorLog(&err_log);
    mapper.enableCaching(true);

    // call the test routines
    test_overlap_regions();

    test_trcid_cache_mem_cb();

    test_mem_spaces();

       
    oss.str("");
    oss << "\n*** Memory access tests complete.***\nPassed: " << tests_passed << "; Failed: " << tests_failed << "\n";
    logger.LogMsg(oss.str());
  	return (tests_failed == 0) ? 0 : -2;
}

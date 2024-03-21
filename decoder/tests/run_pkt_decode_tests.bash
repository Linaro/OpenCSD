#!/bin/bash
#################################################################################
# Copyright 2018 ARM. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, 
# this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice, 
# this list of conditions and the following disclaimer in the documentation 
# and/or other materials provided with the distribution. 
# 
# 3. Neither the name of the copyright holder nor the names of its contributors 
# may be used to endorse or promote products derived from this software without 
# specific prior written permission. 
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND 
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
# 
#################################################################################
# OpenCSD library: Test script.
#
# Test script to run packet lister on each of the snapshots retained with the repository.
# No attempt is made to compare output results to previous versions,  (output formatting
# may change due to bugfix / enhancements) or assess the validity  of the trace output.
#
#################################################################################
# Usage options:-
# * default: run tests on binary + libs in ./bin/linux64/rel
# run_pkt_decode_tests.bash
#
# * use installed opencsd libraries & program
# run_pkt_decode_tests.bash use-installed <options>
#
# * use supplied path for binary + libs (must have trailing /)
# run_pkt_decode_tests.bash -bindir <custom>/<path>/ <options>
#

OUT_DIR=./results
SNAPSHOT_DIR=./snapshots
BIN_DIR=./bin/linux64/rel/

# directories for tests using full decode
declare -a test_dirs_decode=( "juno-ret-stck"
                              "a57_single_step"
                              "bugfix-exact-match"
                              "juno-uname-001"
                              "juno-uname-002"
                              "juno_r1_1"
                              "tc2-ptm-rstk-t32"
                              "trace_cov_a15"
                              "stm_only"
                              "stm_only-2"
                              "stm_only-juno"
                              "stm-issue-27"
                              "TC2"
                              "Snowball"
                              "test-file-mem-offsets"
                              "itm_only_raw"
                              "itm_only_csformat"                              
                            )


echo "Running trc_pkt_lister on snapshot directories."

mkdir -p ${OUT_DIR}

if [ "$1" == "use-installed" ]; then
    BIN_DIR=""
    shift
elif [ "$1" == "-bindir" ]; then
    BIN_DIR=$2
    shift
    shift
fi

echo "Tests using BIN_DIR = ${BIN_DIR}"

if [ "${BIN_DIR}" != "" ]; then
    export LD_LIBRARY_PATH=${BIN_DIR}.
    echo "LD_LIBRARY_PATH set to ${BIN_DIR}"
fi

# === test the decode set ===
for test_dir in "${test_dirs_decode[@]}"
do
    echo "Testing $test_dir..."
    ${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/$test_dir" $@ -decode -no_time_print -logfilename "${OUT_DIR}/$test_dir.ppl"
    echo "Done : Return $?"
done

# === test for debugging issues ===
# juno_r1_1 has fake data that triggers range limit and bad opcode if operating
echo "Test with run limit on..."
export OPENCSD_INSTR_RANGE_LIMIT=100
env | grep OPENCSD
${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/juno_r1_1" $@ -decode -no_time_print -logfilename "${OUT_DIR}/juno_r1_1_rangelimit.ppl"
unset OPENCSD_INSTR_RANGE_LIMIT
echo "Done : Return $?"
env | grep OPENCSD

echo "Test with bad opcode detect on using env var..."
export OPENCSD_ERR_ON_AA64_BAD_OPCODE=1
env | grep OPENCSD
${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/juno_r1_1" $@ -decode -no_time_print -logfilename "${OUT_DIR}/juno_r1_1_badopcode.ppl"
unset OPENCSD_ERR_ON_AA64_BAD_OPCODE
echo "Done : Return $?"
env | grep OPENCSD

echo "Test with bad opcode detect on using flag..."
${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/juno_r1_1" $@ -decode -no_time_print -aa64_opcode_chk -logfilename "${OUT_DIR}/juno_r1_1_badopcode_flag.ppl"
echo "Done : Return $?"

# === test a packet only example ===
echo "Testing init-short-addr..."
${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/init-short-addr" $@ -pkt_mon -no_time_print -logfilename "${OUT_DIR}/init-short-addr.ppl"
echo "Done : Return $?"

# === test the TPIU deformatter ===
echo "Testing a55-test-tpiu..."
${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/a55-test-tpiu" $@ -dstream_format -no_time_print -o_raw_packed -o_raw_unpacked -logfilename "${OUT_DIR}/a55-test-tpiu.ppl"
echo "Done : Return $?"

# === Run uninstalled test programs ===
if [ "$1" != "use-installed" ]; then

    # === test the C-API lib ===
    echo "Testing C-API library"
    ${BIN_DIR}c_api_pkt_print_test -ss_path ${SNAPSHOT_DIR} -decode > /dev/null
    echo "Done : Return $?"
    echo "moving result file."
    mv ./c_api_test.log ./${OUT_DIR}/c_api_test.ppl

    # === run the Frame decoder test ===
    echo "Running Frame demux test"
    ${BIN_DIR}frame-demux-test > /dev/null
    echo "Done : Return $?"
    echo "moving result file."
    mv ./frame_demux_test.ppl ./${OUT_DIR}/.

    # === run the memory accessor tests ===
    echo "Running memacc tests"
    echo "Using memory buffer"
    ${BIN_DIR}mem-buffer-eg   -logfile  -ss_path  ./snapshots  -noprint
    echo "Done : Return $?"
    echo "Using callback function"
    ${BIN_DIR}mem-buffer-eg   -logfile  -ss_path  ./snapshots  -noprint -callback
    echo "Done : Return $?"
    echo "moving result files."
    mv ./mem_buff_demo*.ppl ./${OUT_DIR}/.

    # === run the itm decoder test program ===
    echo "Running ITM decoder test"
    ${BIN_DIR}itm-decode-test -logfilename  "${OUT_DIR}/itm-decode-test.ppl" 
    echo "Done : Return $?"
fi

# === run the itm decoder test program ===
if [ "$1" != "use-installed" ]; then
    echo "Running ITM decoder test"
    ${BIN_DIR}itm-decode-test -decode -logfilename  "${OUT_DIR}/itm-decode-test.ppl" 
    echo "Done : Return $?"
fi

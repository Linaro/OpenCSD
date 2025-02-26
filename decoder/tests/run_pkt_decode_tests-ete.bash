#!/bin/bash
#################################################################################
# Copyright 2019 ARM. All rights reserved.
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
# run_pkt_decode_tests.bash use-installed
#
# * use supplied path for binary + libs (must have trailing /)
# run_pkt_decode_tests.bash <custom>/<path>/
#

OUT_DIR=./results-ete
SNAPSHOT_DIR=./snapshots-ete
BIN_DIR=./bin/linux64/rel/

# directories for tests using full decode
declare -a test_dirs_decode=( "001-ack_test"
                              "002-ack_test_scr"
                              "ete-bc-instr"
                              "ete_ip"
                              "ete-ite-instr"
                              "ete_mem"
                              "ete_spec_1"
                              "ete_spec_2"
                              "ete_spec_3"
                              "ete-wfet"
                              "event_test"
                              "infrastructure"
                              "pauth_lr"
                              "pauth_lr_Rm"
                              "q_elem"
                              "rme_test"
                              "s_9001"
                              "src_addr"
                              "ss_ib_el1ns"
                              "tme_simple"
                              "tme_tcancel"
                              "tme_test"
                              "trace_file_cid_vmid"
                              "trace_file_vmid"
                              "ts_bit64_set"
                              "ts_marker"
                            )

# directories for tests using I_SRC_ADDR_range option
declare -a test_dirs_decode_src_addr_opt=( "002-ack_test_scr"
                              "ete_ip"
                              "src_addr"
                              )

# directories with multi session snapshots
declare -a test_dirs_decode_multi_sess=( "ss_ib_el1ns"
                                         "ete-ite-instr"
                                         "pauth_lr"
                                         "pauth_lr_Rm"                                        
                                         "q_elem"
                                         "rme_test"
                                         "s_9001"
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

for test_dir_n in "${test_dirs_decode_src_addr_opt[@]}"
do
    echo "Testing with -src_addr_n  $test_dir_n..."
    ${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/$test_dir_n" $@ -decode  -no_time_print -src_addr_n -logfilename "${OUT_DIR}/${test_dir_n}_src_addr_N.ppl"
    echo "Done : Return $?"
done

for test_dir_ms in "${test_dirs_decode_multi_sess[@]}"
do
    echo "Testing with -multi_session  $test_dir_ms..."
    ${BIN_DIR}trc_pkt_lister -ss_dir "${SNAPSHOT_DIR}/$test_dir_ms" $@ -decode -no_time_print -multi_session -logfilename "${OUT_DIR}/${test_dir_ms}_multi_sess.ppl"
    echo "Done : Return $?"
done

########################################################
# Copyright 2015 ARM Limited. All rights reserved.
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
# RCTDL - master makefile for libraries and tests
#
# command line options
# DEBUG=1 	    create a debug build
# LINUX64=1     create a 64 bit build

# Set project root - relative to build directory
ifeq ($(RCTDL_ROOT),)
RCTDL_ROOT := $(shell pwd | sed 's,/build/linux.*,,')
export RCTDL_ROOT
endif

# library target directory
export RCTDL_LIB_ROOT=$(RCTDL_ROOT)/lib

export RCTDL_INCLUDE=$(RCTDL_ROOT)/include
export RCTDL_SOURCE=$(RCTDL_ROOT)/source

# tools
export MASTER_CC=gcc
export MASTER_CPP=g++
export MASTER_LINKER=g++
export MASTER_LIB=ar

# compile flags
MASTER_CC_FLAGS := -c -Wall -DLINUX
MASTER_CPP_FLAGS := -c -Wall -DLINUX -std=c++11
MASTER_LINKER_FLAGS := -Wl,-z,defs
MASTER_LIB_FLAGS := rcs

ifdef DEBUG
MASTER_CC_FLAGS += -g -O0 -DDEBUG
MASTER_CPP_FLAGS += -g -O0 -DDEBUG
BUILD_VARIANT=dbg
else
MASTER_CC_FLAGS += -g -O2 -DNDEBUG
MASTER_CPP_FLAGS += -g -O2 -DNDEBUG
BUILD_VARIANT=rel
endif

ifdef LINUX64
MASTER_CC_FLAGS += -m64
MASTER_CPP_FLAGS += -m64
MASTER_LINKER_FLAGS += -m64
BIT_VARIANT=64
else
MASTER_CC_FLAGS += -m32
MASTER_CPP_FLAGS += -m32
MASTER_LINKER_FLAGS += -m32
BIT_VARIANT=32
endif

export MASTER_CC_FLAGS
export MASTER_CPP_FLAGS
export MASTER_LINKER_FLAGS
export MASTER_LIB_FLAGS


export PLAT_DIR=linux$(BIT_VARIANT)/$(BUILD_VARIANT)
export LIB_TARGET_DIR=$(RCTDL_LIB_ROOT)/$(PLAT_DIR)


###########################################################
# build targets

all: libs tests

libs: ref_trace_decode_lib 

#rctdl_c_api_lib


ref_trace_decode_lib: $(LIB_TARGET_DIR)/libref_trace_decode.a

$(LIB_TARGET_DIR)/libref_trace_decode.a:
	mkdir -p $(LIB_TARGET_DIR)
	cd $(RCTDL_ROOT)/build/linux/ref_trace_decode_lib && make

#$(LIB_TARGET_DIR)/libref_trace_decode.a


#	cd $(RCTDL_ROOT)/build/linux/rctdl_c_api_lib && make

tests:
#	cd $(RCTDL_ROOT)/tests/build/linux/snapshot_parser_lib && make
#	cd $(RCTDL_ROOT)/tests/build/linux/trc_pkt_lister && make


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
# RCTDL - makefile for main trace decode library
#

CPP := $(MASTER_CPP)
LINKER := $(MASTER_LINKER)
LIB := $(MASTER_LIB)

CPP_FLAGS := $(MASTER_CPP_FLAGS) -fpic -Wno-switch
LIB_FLAGS := $(MASTER_LIB_FLAGS)

LIB_NAME = libref_trace_decoder

BUILD_DIR=./$(PLAT_DIR)

VPATH=	$(RCTDL_SOURCE) \
		$(RCTDL_SOURCE)/etmv3 \
		$(RCTDL_SOURCE)/etmv4 \
		$(RCTDL_SOURCE)/ptm


CPP_INCLUDES= \
				-I$(RCTDL_INCLUDE) \
				-I$(RCTDL_SOURCE)

OBJECTS=$(BUILD_DIR)/rctdl_dcd_tree.o \
		$(BUILD_DIR)/rctdl_error.o \
		$(BUILD_DIR)/rctdl_error_logger.o \
		$(BUILD_DIR)/rctdl_msg_logger.o \
		$(BUILD_DIR)/trc_cmp_cfg_etmv3.o \
		$(BUILD_DIR)/trc_cmp_cfg_etmv4.o \
		$(BUILD_DIR)/trc_cmp_cfg_ptm.o \
		$(BUILD_DIR)/trc_component.o \
		$(BUILD_DIR)/trc_core_arch_map.o \
		$(BUILD_DIR)/trc_frame_deformatter.o \
		$(BUILD_DIR)/trc_pkt_elem_etmv3.o \
		$(BUILD_DIR)/trc_pkt_elem_etmv4i.o \
		$(BUILD_DIR)/trc_pkt_elem_etmv4d.o \
		$(BUILD_DIR)/trc_pkt_elem_ptm.o \
		$(BUILD_DIR)/trc_pkt_proc_etmv3.o \
		$(BUILD_DIR)/trc_pkt_proc_etmv4.o \
		$(BUILD_DIR)/trc_pkt_proc_etmv4i_impl.o \
		$(BUILD_DIR)/trc_pkt_proc_ptm.o


all: build_dir $(OBJECTS)
	mkdir -p $(LIB_TARGET_DIR)
	$(LIB) $(LIB_FLAGS) $(LIB_TARGET_DIR)/$(LIB_NAME).a $(OBJECTS)

build_dir:
	mkdir -p $(BUILD_DIR)

##### build rules
$(BUILD_DIR)/%.o : %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDES) $< -o $@

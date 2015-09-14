/*
 * \file       trc_mem_acc_mapper.h
 * \brief      Reference CoreSight Trace Decoder : 
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

#ifndef ARM_TRC_MEM_ACC_MAPPER_H_INCLUDED
#define ARM_TRC_MEM_ACC_MAPPER_H_INCLUDED

#include <vector>

#include "rctdl_if_types.h"
#include "interfaces/trc_tgt_mem_access_i.h"
#include "mem_acc/trc_mem_acc_base.h"

typedef enum _memacc_mapper_t {
    MEMACC_MAP_GLOBAL,
} memacc_mapper_t;

class TrcMemAccMapper : public ITargetMemAccess
{
public:
    TrcMemAccMapper();
    TrcMemAccMapper(bool using_trace_id);
    virtual ~TrcMemAccMapper();

    // memory access interface
    virtual rctdl_err_t ReadTargetMemory(   const rctdl_vaddr_t address, 
                                            const uint8_t cs_trace_id, 
                                            const rctdl_mem_space_acc_t mem_space, 
                                            uint32_t *num_bytes, 
                                            uint8_t *p_buffer);
    //virtual rctdl_err_t ReadTargetMemory(const rctdl_vaddr_t address, const rctdl_mem_space_acc_t mem_space, const uint8_t cs_trace_id, uint32_t *num_bytes, uint8_t *p_buffer);

    // mapper configuration interface
    virtual rctdl_err_t AddAccessor(TrcMemAccessorBase *p_accessor, const uint8_t cs_trace_id) = 0;

    // destroy all attached accessors
    void DestroyAllAccessors();



protected:
    virtual bool findAccessor(const rctdl_vaddr_t address, const uint8_t cs_trace_id) = 0;     // set m_acc_curr if found valid range, leave unchanged if not.
    virtual bool readFromCurrent(const rctdl_vaddr_t address, const uint8_t cs_trace_id) = 0;
    virtual TrcMemAccessorBase *getFirstAccessor() = 0;
    virtual TrcMemAccessorBase *getNextAccessor() = 0;
    virtual void clearAccessorList() = 0;

    TrcMemAccessorBase *m_acc_curr;     // most recently used - try this first.
    uint8_t m_trace_id_curr;            // trace ID for the current accessor
    const bool m_using_trace_id;        // true if we are using separate memory spaces by TraceID.
};


// address spaces common to all sources using this mapper.
// trace id unused.
class TrcMemAccMapGlobalSpace : public TrcMemAccMapper
{
public:
    TrcMemAccMapGlobalSpace();
    virtual ~TrcMemAccMapGlobalSpace();

    // mapper creation interface - prevent overlaps
    virtual rctdl_err_t AddAccessor(TrcMemAccessorBase *p_accessor, const uint8_t cs_trace_id);

protected:
    virtual bool findAccessor(const rctdl_vaddr_t address, const uint8_t cs_trace_id); 
    virtual bool readFromCurrent(const rctdl_vaddr_t address, const uint8_t cs_trace_id);    
    virtual TrcMemAccessorBase *getFirstAccessor();
    virtual TrcMemAccessorBase *getNextAccessor();
    virtual void clearAccessorList();

    std::vector<TrcMemAccessorBase *> m_acc_global;
    std::vector<TrcMemAccessorBase *>::iterator m_acc_it;
};

#endif // ARM_TRC_MEM_ACC_MAPPER_H_INCLUDED

/* End of File trc_mem_acc_mapper.h */

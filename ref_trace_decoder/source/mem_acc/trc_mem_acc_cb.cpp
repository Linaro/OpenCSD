/*!
 * \file       trc_mem_acc_cb.cpp
 * \brief      Reference CoreSight Trace Decoder : Trace Mem accessor - callback function
 * 
 * \copyright  Copyright (c) 2015, ARM Limited. All Rights Reserved.
 */

#include "mem_acc/trc_mem_acc_cb.h"

TrcMemAccCB::TrcMemAccCB(const rctdl_vaddr_t s_address, 
                const rctdl_vaddr_t e_address, 
                const rctdl_mem_space_acc_t mem_space) : 
    TrcMemAccessorBase(MEMACC_CB_IF, s_address, e_address),
    m_p_CBclass(0),
    m_p_CBfn(0)
{
    setMemSpace(mem_space);    
}

/** Memory access override - allow decoder to read bytes from the buffer. */
const uint32_t TrcMemAccCB::readBytes(const rctdl_vaddr_t address, const rctdl_mem_space_acc_t memSpace, const uint32_t reqBytes, uint8_t *byteBuffer)
{
    // if we have a callback object, use it to call back.
    if(m_p_CBclass)
        return m_p_CBclass->readBytes(address,memSpace,reqBytes,byteBuffer);
    if(m_p_CBfn)
        return m_p_CBfn(address,memSpace,reqBytes,byteBuffer);
    return 0;
}

/* End of File trc_mem_acc_cb.cpp */

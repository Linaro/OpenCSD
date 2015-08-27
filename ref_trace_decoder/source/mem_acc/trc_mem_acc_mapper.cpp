/*
 * \file       trc_mem_acc_mapper.cpp
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

#include "mem_acc/trc_mem_acc_mapper.h"
#include "mem_acc/trc_mem_acc_file.h"

/************************************************************************************/
/* mappers base class */
/************************************************************************************/

TrcMemAccMapper::TrcMemAccMapper() :
    m_acc_curr(0),
    m_trace_id_curr(0),
    m_using_trace_id(false)
{
}

TrcMemAccMapper::TrcMemAccMapper(bool using_trace_id) : 
    m_acc_curr(0),
    m_trace_id_curr(0),
    m_using_trace_id(using_trace_id)
{
}

TrcMemAccMapper::~TrcMemAccMapper()
{
}

// memory access interface
rctdl_err_t TrcMemAccMapper::ReadTargetMemory(const rctdl_vaddr_t address, const uint8_t cs_trace_id, uint32_t *num_bytes, uint8_t *p_buffer)
{
    bool bReadFromCurr = true;

    /* see if the address is in any range we know */
    if(!readFromCurrent(address, cs_trace_id))
       bReadFromCurr = findAccessor(address, cs_trace_id);

    /* if bReadFromCurr then we know m_acc_curr is set */
    if(bReadFromCurr)
        *num_bytes = m_acc_curr->readBytes(address,*num_bytes,p_buffer);
    else
        *num_bytes = 0;
    return RCTDL_OK;
}

void TrcMemAccMapper::DestroyAllAccessors()
{
    TrcMemAccessorBase *pAcc = 0;
    pAcc = getFirstAccessor();
    while(pAcc != 0)
    {
        switch(pAcc->getType())
        {
        case TrcMemAccessorBase::MEMACC_FILE:
            TrcMemAccessorFile::destroyFileAccessor(dynamic_cast<TrcMemAccessorFile *>(pAcc));
            break;

        case TrcMemAccessorBase::MEMACC_BUFPTR:
            delete pAcc;
            break;

        default:
            break;

        }
    }
    clearAccessorList();
}

/************************************************************************************/
/* mappers global address space class - no differentiation in core trace IDs */
/************************************************************************************/
TrcMemAccMapGlobalSpace::TrcMemAccMapGlobalSpace() : TrcMemAccMapper()
{
}

TrcMemAccMapGlobalSpace::~TrcMemAccMapGlobalSpace()
{
}

rctdl_err_t TrcMemAccMapGlobalSpace::AddAccessor(TrcMemAccessorBase *p_accessor, const uint8_t /*cs_trace_id*/)
{
    rctdl_err_t err = RCTDL_OK;
    bool bOverLap = false;
    std::vector<TrcMemAccessorBase *>::const_iterator it =  m_acc_global.begin();
    while((it != m_acc_global.end()) && !bOverLap)
    {
        // if overlap and memory space match
        if( ((*it)->overLapRange(p_accessor)) &&
            ((*it)->inMemSpace(p_accessor->getMemSpace()))
            )
        {
            bOverLap = true;
            err = RCTDL_ERR_MEM_ACC_OVERLAP;
        }
        it++;
    }

    // no overlap - add to the list of ranges.
    if(!bOverLap)
        m_acc_global.push_back(p_accessor);

    return err;
}

bool TrcMemAccMapGlobalSpace::findAccessor(const rctdl_vaddr_t address, const uint8_t /*cs_trace_id*/)
{
    bool bFound = false;
    std::vector<TrcMemAccessorBase *>::const_iterator it =  m_acc_global.begin();
    while((it != m_acc_global.end()) && !bFound)
    {
        if((*it)->addrInRange(address))
        {
            bFound = true;
            m_acc_curr = *it;
        }
        it++;
    }
    return bFound;
}

bool TrcMemAccMapGlobalSpace::readFromCurrent(const rctdl_vaddr_t address, const uint8_t /*cs_trace_id*/)
{
    bool readFromCurr = false;
    if(m_acc_curr)
        readFromCurr = m_acc_curr->addrInRange(address);
    return readFromCurr;
}


TrcMemAccessorBase * TrcMemAccMapGlobalSpace::getFirstAccessor()
{
    m_acc_it = m_acc_global.begin();
    return getNextAccessor();
}

TrcMemAccessorBase *TrcMemAccMapGlobalSpace::getNextAccessor()
{
    TrcMemAccessorBase *p_acc = 0;
    if(m_acc_it != m_acc_global.end())
    {
        p_acc = *m_acc_it;
        m_acc_it++;
    }
    return p_acc;
}

void TrcMemAccMapGlobalSpace::clearAccessorList()
{
    m_acc_global.clear();
}


/* End of File trc_mem_acc_mapper.cpp */

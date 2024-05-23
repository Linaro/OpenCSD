/*!
* \file       trc_mem_acc_cache.cpp
* \brief      OpenCSD : Memory accessor cache.
*
* \copyright  Copyright (c) 2018, ARM Limited. All Rights Reserved.
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

#include <cstring>
#include <sstream>
#include <iomanip>
#include "mem_acc/trc_mem_acc_cache.h"
#include "mem_acc/trc_mem_acc_base.h"
#include "interfaces/trc_error_log_i.h"
#include "common/ocsd_error.h"

#ifdef LOG_CACHE_STATS
#define INC_HITS_RL(idx) m_hits++; m_hit_rl[m_mru_idx]++;
#define INC_MISS() m_misses++;
#define INC_PAGES() m_pages++;
#define SET_MAX_RL(idx)                         \
    {                                           \
        if (m_hit_rl_max[idx] < m_hit_rl[idx])  \
            m_hit_rl_max[idx] = m_hit_rl[idx];  \
        m_hit_rl[idx] = 0;                      \
    }
#define INC_RL(idx) m_hit_rl[m_mru_idx]++;
#else
#define INC_HITS_RL(idx)
#define INC_MISS() 
#define INC_PAGES() 
#define SET_MAX_RL(idx)
#define INC_RL(idx)  
#endif

// uncomment to log cache ops
// #define LOG_CACHE_OPS
// #define LOG_CACHE_CREATION

ocsd_err_t TrcMemAccCache::createCaches()
{
    if (m_mru)
        destroyCaches();
    m_mru = (cache_block_t*) new (std::nothrow) cache_block_t[m_mru_num_pages];
    if (!m_mru)
        return OCSD_ERR_MEM;
    for (int i = 0; i < m_mru_num_pages; i++) {
        m_mru[i].data = new (std::nothrow) uint8_t[m_mru_page_size];
        if (!m_mru[i].data)
            return OCSD_ERR_MEM;
        clearPage(&m_mru[i]);
    }
#ifdef LOG_CACHE_STATS
    m_hit_rl = (uint32_t *) new (std::nothrow) uint32_t[m_mru_num_pages];
    m_hit_rl_max = (uint32_t*) new (std::nothrow) uint32_t[m_mru_num_pages];
    if (!m_hit_rl || !m_hit_rl_max)
        return OCSD_ERR_MEM;
    for (int j = 0; j < m_mru_num_pages; j++) {
        m_hit_rl[j] = 0;
        m_hit_rl_max[j] = 0;
    }
#endif
#ifdef LOG_CACHE_CREATION
    std::ostringstream oss;
    oss << "MemAcc Caches: Num Pages=" << m_mru_num_pages << "; Page size=" << m_mru_page_size << ";\n";
    logMsg(oss.str());
#endif
    return OCSD_OK;
}

void TrcMemAccCache::destroyCaches()
{
    if (m_mru) {
        for (int i = 0; i < m_mru_num_pages; i++)
            delete[] m_mru[i].data;
        delete[] m_mru;
        m_mru = 0;
    }
#ifdef LOG_CACHE_STATS
    if (m_hit_rl)
        delete[] m_hit_rl;
    if (m_hit_rl_max)
        delete[] m_hit_rl_max;
    m_hit_rl = 0;
    m_hit_rl_max = 0;
#endif

}

void TrcMemAccCache::getenvMemaccCacheSizes(bool& enable, int& page_size, int& num_pages)
{
    char* env_var;
    long env_val;

    /* set defaults */
    enable = true;
    page_size = MEM_ACC_CACHE_DEFAULT_PAGE_SIZE;
    num_pages = MEM_ACC_CACHE_DEFAULT_MRU_SIZE;

    /* check environment for adjustments */

    /* override the default on switch? if so no need to look further */
    if ((env_var = getenv(OCSD_ENV_MEMACC_CACHE_OFF)) != NULL)
    {
        enable = false;
        return;
    }

    /* check for tweak in page size */
    if ((env_var = getenv(OCSD_ENV_MEMACC_CACHE_PG_SIZE)) != NULL)
    {
        env_val = strtol(env_var, NULL, 0);
        /*
         * if no valid conversion then env_val = 0,
         * otherwise set val and allow TrcMemAccCache::setCacheSizes
         * fn to ensure the value in bounds
         */
        if (env_val > 0)
            page_size = (int)env_val;
    }

    /* check for tweak in number of pages */
    if ((env_var = getenv(OCSD_ENV_MEMACC_CACHE_PG_NUM)) != NULL)
    {
        env_val = strtol(env_var, NULL, 0);
        /*
         * if no valid conversion then env_val = 0,
         * otherwise set val and allow TrcMemAccCache::setCacheSizes
         * fn to ensure the value in bounds
         */
        if (env_val > 0)
            num_pages = (int)env_val;
    }

}

ocsd_err_t TrcMemAccCache::enableCaching(bool bEnable)
{
    ocsd_err_t err = OCSD_OK;

    if (bEnable)
    {
        // don't create caches if they are done already.
        if (!m_mru)
            err = createCaches();
    }
    else
        destroyCaches();
    m_bCacheEnabled = bEnable;

#ifdef LOG_CACHE_CREATION
    std::ostringstream oss;
    oss << "MemAcc Caches: " << (bEnable ? "Enabled" : "Disabled") << ";\n";
    logMsg(oss.str());
#endif

    return err;
}

ocsd_err_t TrcMemAccCache::setCacheSizes(const uint16_t page_size, const int nr_pages, const bool err_on_limit /*= false*/)
{
    // do't re-create what we already have.
    if (m_mru &&
        (m_mru_num_pages == nr_pages) &&
        (m_mru_page_size == page_size))
        return OCSD_OK;

    /* remove any caches with the existing sizes */
    destroyCaches();

    /* set page size within Max/Min range */
    if (page_size > MEM_ACC_CACHE_PAGE_SIZE_MAX)
    {
        if (err_on_limit)
        {
            logMsg("MemAcc Caching: page size too large", OCSD_ERR_INVALID_PARAM_VAL);
            return OCSD_ERR_INVALID_PARAM_VAL;
        }
        m_mru_page_size = MEM_ACC_CACHE_PAGE_SIZE_MAX;
    }
    else if (page_size < MEM_ACC_CACHE_PAGE_SIZE_MIN)
    {
        if (err_on_limit)
        {
            logMsg("MemAcc Caching: page size too small", OCSD_ERR_INVALID_PARAM_VAL);
            return OCSD_ERR_INVALID_PARAM_VAL;
        }
        m_mru_page_size = MEM_ACC_CACHE_PAGE_SIZE_MIN;
    }
    else
        m_mru_page_size = page_size;

    /* set num pages within max/min range */
    if (nr_pages > MEM_ACC_CACHE_MRU_SIZE_MAX)
    {
        if (err_on_limit)
        {
            logMsg("MemAcc Caching: number of pages too large", OCSD_ERR_INVALID_PARAM_VAL);
            return OCSD_ERR_INVALID_PARAM_VAL;
        }
        m_mru_num_pages = MEM_ACC_CACHE_MRU_SIZE_MAX;
    }
    else if (nr_pages < MEM_ACC_CACHE_MRU_SIZE_MIN)
    {
        if (err_on_limit)
        {
            logMsg("MemAcc Caching: number of pages too small", OCSD_ERR_INVALID_PARAM_VAL);
            return OCSD_ERR_INVALID_PARAM_VAL;
        }
        m_mru_num_pages = MEM_ACC_CACHE_MRU_SIZE_MIN;
    }
    else
        m_mru_num_pages = nr_pages;

    /* re-create with new sizes */
    return createCaches();
}

/* return index of unused page, or oldest used page by sequence number */
int TrcMemAccCache::findNewPage()
{
    uint32_t oldest_seq;
    int current_idx, oldest_idx;
#ifdef LOG_CACHE_OPS
    std::ostringstream oss;
#endif

    /* set up search indexes and check the current search index has not wrapped. */
    current_idx = m_mru_idx + 1;
    oldest_idx = m_mru_idx;
    oldest_seq = 0;
    if (current_idx >= m_mru_num_pages)
        current_idx = 0;

    /* search forwards from m_mru_idx + 1 until we wrap and hit the index again */
    while (current_idx != m_mru_idx) {
        if (m_mru[current_idx].use_sequence == 0) {
#ifdef LOG_CACHE_OPS
            oss << "TrcMemAccCache:: ALI-allocate clean page:  [page: " << std::dec << current_idx << "]\n";
            logMsg(oss.str());
#endif
            return current_idx;
        }

        // if we find a page with a lower use sequence, that is older.
        if ((oldest_seq == 0) || (oldest_seq > m_mru[current_idx].use_sequence)) {
            oldest_seq = m_mru[current_idx].use_sequence;
            oldest_idx = current_idx;
        }

        current_idx++;

        // wrap around?
        if (current_idx >= m_mru_num_pages)
            current_idx = 0;
    }
#ifdef LOG_CACHE_OPS
    oss << "TrcMemAccCache:: ALI-evict and allocate old page:  [page: " << std::dec << oldest_idx << "]\n";
    logMsg(oss.str());
#endif
    return oldest_idx;
}

void TrcMemAccCache::incSequence()
{
    m_mru[m_mru_idx].use_sequence = m_mru_sequence++;
    if (m_mru_sequence == 0) {
        // wrapped which throws out the oldest algorithm - oldest will now appear newer - so not evicted.
        // arbitrarily re-sequence all in use..
        m_mru_sequence = 1;
        for (int i = 0; i < m_mru_num_pages; i++)
            if (m_mru[i].use_sequence != 0)
                m_mru[i].use_sequence = m_mru_sequence++;

        // ensure newest still newest...
        m_mru[m_mru_idx].use_sequence = m_mru_sequence++;
    }
}

ocsd_err_t TrcMemAccCache::readBytesFromCache(TrcMemAccessorBase *p_accessor, const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t trcID, uint32_t *numBytes, uint8_t *byteBuffer)
{
    uint32_t bytesRead = 0, reqBytes = *numBytes;
    ocsd_err_t err = OCSD_OK;
    

#ifdef LOG_CACHE_OPS
    std::ostringstream oss;
    std::string memSpaceStr;
#endif

    if (m_bCacheEnabled)
    {
        if (blockInCache(address, reqBytes, trcID))
        {
            bytesRead = reqBytes;
            memcpy(byteBuffer, &m_mru[m_mru_idx].data[address - m_mru[m_mru_idx].st_addr], reqBytes);
            incSequence();
#ifdef LOG_CACHE_OPS
            oss << "TrcMemAccCache:: hit {page: " << std::dec << m_mru_idx << "; seq: " << m_mru[m_mru_idx].use_sequence << " CSID: " << std::hex << (int)m_mru[m_mru_idx].trcID;
            oss << "} [addr:0x" << std::hex << address << ", bytes: " << std::dec << reqBytes << "]\n";
            logMsg(oss.str());
#endif
            INC_HITS_RL(m_mru_idx);
        }
        else
        {
#ifdef LOG_CACHE_OPS
            oss << "TrcMemAccCache:: miss [addr:0x" << std::hex << address << ", bytes: " << std::dec << reqBytes << "]\n";
            logMsg(oss.str());
#endif
            /* need a new cache page - check the underlying accessor for the data */
            m_mru_idx = findNewPage();
            m_mru[m_mru_idx].valid_len = p_accessor->readBytes(address, mem_space, trcID, m_mru_page_size, &m_mru[m_mru_idx].data[0]);
            
            /* check return length valid - v bad if return length more than request */
            if (m_mru[m_mru_idx].valid_len > m_mru_page_size)
            {
                m_mru[m_mru_idx].valid_len = 0; // set to nothing returned.
                err = OCSD_ERR_MEM_ACC_BAD_LEN;
            }
            
            if (m_mru[m_mru_idx].valid_len > 0)
            {
                // got some data - so save the details                
                m_mru[m_mru_idx].st_addr = address;
                m_mru[m_mru_idx].trcID = trcID;
                incSequence();

                // log the run length hit counts
                SET_MAX_RL(m_mru_idx);
                
#ifdef LOG_CACHE_OPS
                TrcMemAccessorBase::getMemAccSpaceString(memSpaceStr, mem_space);
                oss.str("");
                oss << "TrcMemAccCache:: ALI-load {page: " << std::dec << m_mru_idx << "; seq: " << m_mru[m_mru_idx].use_sequence << " CSID: " << std::hex << (int)m_mru[m_mru_idx].trcID;
                oss << "} [mem space: " << memSpaceStr << ", addr:0x" << std::hex << address << ", bytes: " << std::dec << m_mru[m_mru_idx].valid_len << "]\n";
                logMsg(oss.str());
#endif
                INC_PAGES();              

                if (blockInPage(address, reqBytes, trcID)) /* check we got the data we needed */
                {
                    bytesRead = reqBytes;
                    memcpy(byteBuffer, &m_mru[m_mru_idx].data[address - m_mru[m_mru_idx].st_addr], reqBytes);
                    INC_RL(m_mru_idx);
                }
                else
                {
#ifdef LOG_CACHE_OPS
                    oss.str("");
                    oss << "TrcMemAccCache:: miss-after-load {page: " << std::dec << m_mru_idx << " } [addr:0x" << std::hex << address << ", bytes: " << std::dec << m_mru[m_mru_idx].valid_len << "]\n";
                    logMsg(oss.str());
#endif
                    INC_MISS();
                }
            }
        }
    }
    *numBytes = bytesRead;
    return err;
}

void TrcMemAccCache::invalidateAll()
{
#ifdef LOG_CACHE_OPS
    std::ostringstream oss;
    oss << "TrcMemAccCache:: ALI-invalidate All\n";
    logMsg(oss.str());
#endif

    for (int i = 0; i < m_mru_num_pages; i++)
        clearPage(&m_mru[i]);
    m_mru_idx = 0;
}

void TrcMemAccCache::invalidateByTraceID(int8_t trcID)
{
#ifdef LOG_CACHE_OPS
    std::ostringstream oss;
    oss << "TrcMemAccCache:: ALI-invalidate by ID request {CSID: " << std::hex << (int)trcID << "}\n";
    logMsg(oss.str());
#endif

    for (int i = 0; i < m_mru_num_pages; i++)
    {
        if (m_mru[i].trcID == trcID)
        {
#ifdef LOG_CACHE_OPS
            oss.str("");
            oss << "TrcMemAccCache:: ALI-invalidate page {page: " << std::dec << i << "; seq: " << m_mru[i].use_sequence << " CSID: " << std::hex << (int)m_mru[i].trcID;
            oss << "} [addr:0x" << std::hex << m_mru[i].st_addr << ", bytes: " << std::dec << m_mru[i].valid_len << "]\n";
            logMsg(oss.str());
#endif
            clearPage(&m_mru[i]);
        }
    }
}

void TrcMemAccCache::logMsg(const std::string &szMsg, ocsd_err_t err /*= OCSD_OK */ )
{
    if (m_err_log)
    {
        if (err == OCSD_OK)
            m_err_log->LogMessage(ITraceErrorLog::HANDLE_GEN_INFO, OCSD_ERR_SEV_INFO, szMsg);
        else
        {
            ocsdError ocsd_err( OCSD_ERR_SEV_ERROR, err, szMsg);
            m_err_log->LogError(ITraceErrorLog::HANDLE_GEN_INFO, &ocsd_err);
        }
    }
}

void TrcMemAccCache::setErrorLog(ITraceErrorLog *log)
{
    m_err_log = log;
}

void TrcMemAccCache::logAndClearCounts()
{
#ifdef LOG_CACHE_STATS
    std::ostringstream oss;

    oss << "TrcMemAccCache:: cache performance: Page Size: 0x" << std::hex << m_mru_page_size << "; Number of Pages: " << std::dec << m_mru_num_pages << "\n";
    oss << "Cache hits(" << std::dec << m_hits << "), misses(" << m_misses << "), new pages(" << m_pages << ")\n";
    logMsg(oss.str());
    for (int i = 0; i < m_mru_num_pages; i++)
    {
        if (m_hit_rl_max[i] < m_hit_rl[i])
            m_hit_rl_max[i] = m_hit_rl[i];
        oss.str("");
        oss << "Run length max page " << std::dec << i << ": " << m_hit_rl_max[i] << "\n";
        logMsg(oss.str());
    }
    m_hits = m_misses = m_pages = 0;
#endif
}

/* End of File trc_mem_acc_cache.cpp */

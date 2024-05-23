/*!
* \file       trc_mem_acc_cache.h
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

#ifndef ARM_TRC_MEM_ACC_CACHE_H_INCLUDED
#define ARM_TRC_MEM_ACC_CACHE_H_INCLUDED

#include <string>
#include "opencsd/ocsd_if_types.h"

#define MEM_ACC_CACHE_DEFAULT_PAGE_SIZE 2048
#define MEM_ACC_CACHE_DEFAULT_MRU_SIZE 16
#define MEM_ACC_CACHE_PAGE_SIZE_MAX 16384
#define MEM_ACC_CACHE_MRU_SIZE_MAX 256
#define MEM_ACC_CACHE_PAGE_SIZE_MIN 64
#define MEM_ACC_CACHE_MRU_SIZE_MIN 4

#define OCSD_ENV_MEMACC_CACHE_OFF "OPENCSD_MEMACC_CACHE_OFF"
#define OCSD_ENV_MEMACC_CACHE_PG_SIZE "OPENCSD_MEMACC_CACHE_PAGE_SIZE"
#define OCSD_ENV_MEMACC_CACHE_PG_NUM  "OPENCSD_MEMACC_CACHE_PAGE_NUM"


class TrcMemAccessorBase;
class ITraceErrorLog;

typedef struct cache_block {
    ocsd_vaddr_t st_addr;
    uint32_t valid_len;
    uint8_t* data;
    uint8_t trcID;          // trace ID associated with the page
    uint32_t use_sequence; // number representing the sequence of allocation to evict oldest page.
} cache_block_t;

// enable define to collect stats for debugging / cache performance tests
// #define LOG_CACHE_STATS


/** class TrcMemAccCache - cache small amounts of data from accessors to speed up decode.
 * 
 * Reduce the need to read files / make callbacks into clients when walking memory images.
 * 
 * Caching is done on a per Core/Trace ID basis - all caches from that ID are invalidated when a context
 * switch appears on the core. This means that we do not account for memory spaces in the cache pages as
 * these only change via a context switch.
 * 
 * Memory space is used on cache miss if reading data from the underlying accessor (file / callback).
 */
class TrcMemAccCache
{
public:
    TrcMemAccCache();
    ~TrcMemAccCache();

    /* cache enabling and usage */
    ocsd_err_t enableCaching(bool bEnable);
    // optionally error if outside limits - otherwise set to max / min automatically
    ocsd_err_t setCacheSizes(const uint16_t page_size, const int nr_pages, const bool err_on_limit = false);

    const bool enabled() const { return m_bCacheEnabled; };
    const bool enabled_for_size(const uint32_t reqSize) const
    {
        return (m_bCacheEnabled && (reqSize <= m_mru_page_size));
    }

    /* cache invalidation */
    void invalidateAll();
    void invalidateByTraceID(int8_t trcID);
    void clearPage(cache_block_t* page);

    /** read bytes from cache if possible - load new page if needed from underlying accessor, bail out if data not available */
    ocsd_err_t readBytesFromCache(TrcMemAccessorBase *p_accessor, const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t trcID, uint32_t *numBytes, uint8_t *byteBuffer);

    void setErrorLog(ITraceErrorLog *log);
    void logAndClearCounts();

    /* look for runtime cache tuning vars */
    static void getenvMemaccCacheSizes(bool& enable, int& page_size, int& num_pages);

private:
    bool blockInCache(const ocsd_vaddr_t address, const uint32_t reqBytes, const uint8_t trcID); // run through each page to look for data.
    bool blockInPage(const ocsd_vaddr_t address, const uint32_t reqBytes, const uint8_t trcID);    

    void logMsg(const std::string &szMsg, ocsd_err_t err = OCSD_OK);
    int findNewPage();
    void incSequence(); // increment sequence on current block

    ocsd_err_t createCaches();     // create caches according to current sizes 
    void destroyCaches();   // destroy the cache blocks

    cache_block_t *m_mru;       // cache pages 
    int m_mru_idx = 0;          // in use index - most recently used page   
    uint16_t m_mru_page_size;   // page size
    int m_mru_num_pages;        // number of pages  
    uint32_t m_mru_sequence;    // allocation & use sequence number

    bool m_bCacheEnabled = false;

#ifdef LOG_CACHE_STATS    
    uint32_t m_hits = 0;
    uint32_t m_misses = 0;
    uint32_t m_pages = 0;
    uint32_t* m_hit_rl = 0;
    uint32_t* m_hit_rl_max = 0;
#endif
    
    ITraceErrorLog *m_err_log = 0;
};

inline TrcMemAccCache::TrcMemAccCache() :
    m_mru(0), m_mru_sequence(1)
{
    /* set default cache sizes */
    m_mru_page_size = MEM_ACC_CACHE_DEFAULT_PAGE_SIZE;
    m_mru_num_pages = MEM_ACC_CACHE_DEFAULT_MRU_SIZE;
}

inline TrcMemAccCache::~TrcMemAccCache()
{
    destroyCaches();
}


inline bool TrcMemAccCache::blockInPage(const ocsd_vaddr_t address, const uint32_t reqBytes, const uint8_t trcID)
{
    /* check has data, trcID and mem space */
    if ((m_mru[m_mru_idx].trcID != trcID) ||
        (m_mru[m_mru_idx].valid_len == 0)
        )
        return false;

    /* check block is in this page */
    if ((m_mru[m_mru_idx].st_addr <= address) &&
        m_mru[m_mru_idx].st_addr + m_mru[m_mru_idx].valid_len >= (address + reqBytes))
        return true;
    return false;
}

inline bool TrcMemAccCache::blockInCache(const ocsd_vaddr_t address, const uint32_t reqBytes, const uint8_t trcID)
{
    int tests = m_mru_num_pages;
    while (tests)
    {        
        if (blockInPage(address, reqBytes, trcID))
            return true; // found address in page
#ifdef LOG_CACHE_STATS    
        // miss counts of current page only - to determine if we hit other page
        if (tests == m_mru_num_pages)
            m_misses++;
#endif

        tests--;
        m_mru_idx++;
        if (m_mru_idx == m_mru_num_pages)
            m_mru_idx = 0;
    }
    return false;
}

// zero out page parameters rendering it empty
inline void TrcMemAccCache::clearPage(cache_block_t* page)
{
    page->use_sequence = 0;
    page->st_addr = 0;
    page->valid_len = 0;
    page->trcID = OCSD_BAD_CS_SRC_ID;
}

#endif // ARM_TRC_MEM_ACC_CACHE_H_INCLUDED

/* End of File trc_mem_acc_cache.h */

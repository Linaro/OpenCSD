/*
 * \file       trc_mem_acc_file.cpp
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

#include "mem_acc/trc_mem_acc_file.h"

/***************************************************/
/* protected construction and reference counting   */
/***************************************************/

TrcMemAccessorFile::TrcMemAccessorFile() : TrcMemAccessorBase(MEMACC_FILE)
{
    m_ref_count = 0;
}

TrcMemAccessorFile::~TrcMemAccessorFile()
{
    if(m_mem_file.is_open())
        m_mem_file.close();
}

bool TrcMemAccessorFile::initAccessor(const std::string &pathToFile, rctdl_vaddr_t startAddr, size_t offset)
{
    bool init = false;

    m_mem_file.open(pathToFile.c_str(), std::ifstream::binary | std::ifstream::ate);
    if(m_mem_file.is_open())
    {
        m_file_size = (rctdl_vaddr_t)m_mem_file.tellg();
        m_mem_file.seekg(0, m_mem_file.beg);
        if(offset == 0)
            setRange(startAddr,startAddr+m_file_size-1);
        else
        {
            
        }
        m_file_path = pathToFile;
        init = true;
    }
    return init;
}

/***************************************************/
/* static object creation                          */
/***************************************************/

std::map<std::string, TrcMemAccessorFile *> TrcMemAccessorFile::s_FileAccessorMap;

// return existing or create new accessor
TrcMemAccessorFile *TrcMemAccessorFile::createFileAccessor(const std::string &pathToFile, rctdl_vaddr_t startAddr, size_t offset)
{
    TrcMemAccessorFile *p_acc = 0;
    std::map<std::string, TrcMemAccessorFile *>::iterator it = s_FileAccessorMap.find(pathToFile);
    if(it != s_FileAccessorMap.end())
    {
        p_acc = it->second;
        p_acc->IncRefCount();
    }
    else
    { 
        p_acc = new (std::nothrow) TrcMemAccessorFile();
        if(p_acc != 0)
        {
            if(p_acc->initAccessor(pathToFile,startAddr, offset))
            {
                p_acc->IncRefCount();
                s_FileAccessorMap.insert(std::pair<std::string, TrcMemAccessorFile *>(pathToFile,p_acc));
            }
            else
            {
                delete p_acc;
                p_acc = 0;
            }
        }
    }
    return p_acc;
}

void TrcMemAccessorFile::destroyFileAccessor(TrcMemAccessorFile *p_accessor)
{
    if(p_accessor != 0)
    {
        p_accessor->DecRefCount();
        if(p_accessor->getRefCount() == 0)
        {
            std::map<std::string, TrcMemAccessorFile *>::iterator it = s_FileAccessorMap.find(p_accessor->getFilePath());
            if(it != s_FileAccessorMap.end())
            {
                s_FileAccessorMap.erase(it);
            }
            delete p_accessor;
        }
    }
}

const bool TrcMemAccessorFile::isExistingFileAccessor(const std::string &pathToFile)
{
    bool bExists = false;
    std::map<std::string, TrcMemAccessorFile *>::const_iterator it = s_FileAccessorMap.find(pathToFile);
    if(it != s_FileAccessorMap.end())
        bExists = true;
    return bExists;
}

/***************************************************/
/* accessor instance functions                     */
/***************************************************/
const uint32_t TrcMemAccessorFile::readBytes(const rctdl_vaddr_t address, const uint32_t reqBytes, uint8_t *byteBuffer)
{
    if(!m_mem_file.is_open())
        return 0;

    uint32_t bytesRead = bytesInRange(address,reqBytes);    // get avialable bytes in range.
    if(bytesRead)
    {
        rctdl_vaddr_t addr_pos = (rctdl_vaddr_t)m_mem_file.tellg();
        if((address - m_startAddress) != addr_pos)
            m_mem_file.seekg(address - m_startAddress);
        m_mem_file.read((char *)byteBuffer,bytesRead);
    }
    return bytesRead;
}

/* End of File trc_mem_acc_file.cpp */

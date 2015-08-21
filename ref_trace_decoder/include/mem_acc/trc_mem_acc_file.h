/*
 * \file       trc_mem_acc_file.h
 * \brief      Reference CoreSight Trace Decoder :  Access binary target memory file
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

#ifndef ARM_TRC_MEM_ACC_FILE_H_INCLUDED
#define ARM_TRC_MEM_ACC_FILE_H_INCLUDED

#include <map>
#include <string>
#include <fstream>

#include "mem_acc/trc_mem_acc_base.h"

/*!
 * @class TrcMemAccessorFile   
 * @brief Memory accessor for a binary file.
 * 
 * Memory accessor based on a binary file snapshot of some memory. 
 * 
 * Static creation code to allow reference counted accessor usable for 
 * multiple access maps attached to multiple source trees for the same system.
 */
class TrcMemAccessorFile : public TrcMemAccessorBase 
{
public:
    /** read bytes override - reads from file */
    virtual const uint32_t readBytes(const rctdl_vaddr_t address, const uint32_t reqBytes, uint8_t *byteBuffer);

protected:
    TrcMemAccessorFile();   /**< protected default constructor */
    virtual ~ TrcMemAccessorFile(); /**< protected default destructor */

    /** increment reference counter */
    void IncRefCount() { m_ref_count++; };

    /** decrement reference counter */
    void DecRefCount() { m_ref_count--; };

    /** get current reference count */
    const int getRefCount() const { return  m_ref_count; };
        
    /*!
     * Initialise accessor with file name and path, and start address.
     * File opened and length calculated to determine end address for the range.
     *
     * @param &pathToFile : Binary file path and name
     * @param startAddr : system memory address associated with start of binary datain file.
     *
     * @return bool  : true if set up successfully, false if file could not be opened.
     */
    bool initAccessor(const std::string &pathToFile, rctdl_vaddr_t startAddr);

    /** get the file path */
    const std::string &getFilePath() const { return m_file_path; };

public:

    /*!
     * Create a file accessor based on the supplied path and address.
     * Keeps a list of file accessors created.
     *
     * File will be checked to ensure valid accessor can be created.
     *
     * If an accessor using the supplied file is currently in use then a reference to that
     * accessor will be returned and the accessor reference counter updated.
     *
     * @param &pathToFile : Path to binary file
     * @param startAddr : Start address of data represented by file.
     *
     * @return TrcMemAccessorFile * : pointer to accessor if successful, 0 if it could not be created.
     */
    static TrcMemAccessorFile *createFileAccessor(const std::string &pathToFile, rctdl_vaddr_t startAddr);

    /*!
     * Destroy supplied accessor. 
     * 
     * Reference counter decremented and checked and accessor destroyed if no longer in use.
     *
     * @param *p_accessor : File Accessor to destroy.
     */
    static void destroyFileAccessor(TrcMemAccessorFile *p_accessor);

    /*!
     * Test if any accessor is currently using the supplied file path
     *
     * @param &pathToFile : Path to test.
     *
     * @return bool : true if an accessor exists with this file path.
     */
    static const bool isExistingFileAccessor(const std::string &pathToFile);

private:
    static std::map<std::string, TrcMemAccessorFile *> s_FileAccessorMap;   /**< map of file accessors in use. */

private:
    std::ifstream m_mem_file;   /**< input binary file stream */
    int m_ref_count;            /**< accessor reference count */
    std::string m_file_path;    /**< path to input file */
};

#endif // ARM_TRC_MEM_ACC_FILE_H_INCLUDED

/* End of File trc_mem_acc_file.h */

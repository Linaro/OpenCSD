/*
 * \file       rctdl_msg_logger.h
 * \brief      Reference CoreSight Trace Decoder : Generic Message logger
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

#ifndef ARM_RCTDL_MSG_LOGGER_H_INCLUDED
#define ARM_RCTDL_MSG_LOGGER_H_INCLUDED

#include <string>
#include <fstream>

class rctdlMsgLogger 
{
public:
    rctdlMsgLogger();
    ~rctdlMsgLogger();

    typedef enum {
        OUT_NONE = 0,
        OUT_FILE = 1,
        OUT_STDERR = 2,
        OUT_STDOUT = 4
    } output_dest;

    void setLogOpts(int logOpts);
    void setLogFileName(const char *fileName);

    void LogMsg(const std::string &msg);

    const bool isLogging() const;

private:
    int m_outFlags;

    std::string m_logFileName;
    std::fstream m_out_file;
};

#endif // ARM_RCTDL_MSG_LOGGER_H_INCLUDED

/* End of File rctdl_msg_logger.h */

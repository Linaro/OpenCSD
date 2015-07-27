/*
 * \file       rctdl_msg_logger.cpp
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

#include "rctdl_msg_logger.h"

#include <iostream>
#include <sstream>

rctdlMsgLogger::rctdlMsgLogger() :
    m_outFlags(rctdlMsgLogger::OUT_STDOUT),
    m_logFileName("rctdl_trace_decode.log")
{
}

rctdlMsgLogger::~rctdlMsgLogger()
{
    m_out_file.close();
}

void rctdlMsgLogger::setLogOpts(int logOpts)
{
    m_outFlags = logOpts & (rctdlMsgLogger::OUT_FILE | rctdlMsgLogger::OUT_STDERR | rctdlMsgLogger::OUT_STDOUT);
}

void rctdlMsgLogger::setLogFileName(const char *fileName)
{
    m_logFileName = fileName;
    if(m_out_file.is_open())
        m_out_file.close();
}


void rctdlMsgLogger::LogMsg(const std::string &msg)
{
    if(m_outFlags & OUT_STDOUT)
    {
        std::cout << msg << std::endl;
        std::cout.flush();
    }

    if(m_outFlags & OUT_STDERR)
    {
        std::cerr << msg << std::endl;
        std::cerr.flush();
    }

    if(m_outFlags & OUT_FILE)
    {
        if(!m_out_file.is_open())
        {
            m_out_file.open(m_logFileName.c_str(),std::fstream::out | std::fstream::app);
        }
        m_out_file << msg << std::endl;
        m_out_file.flush();
    }
}

const bool rctdlMsgLogger::isLogging() const
{
    return (bool)((m_outFlags & (rctdlMsgLogger::OUT_FILE | rctdlMsgLogger::OUT_STDERR | rctdlMsgLogger::OUT_STDOUT)) != 0);
}

/* End of File rctdl_msg_logger.cpp */

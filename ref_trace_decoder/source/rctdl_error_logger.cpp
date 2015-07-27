/*
 * \file       rctdl_error_logger.cpp
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

#include "rctdl_error_logger.h"

#include <iostream>
#include <sstream>

rctdlDefaultErrorLogger::rctdlDefaultErrorLogger() :
    m_Verbosity(RCTDL_ERR_SEV_INFO),
    m_logger(0),
    m_created_logger(false)
{
    m_lastErr = 0;
    for(int i = 0; i < 0x80; i++)
        m_lastErrID[i] = 0;
    m_error_sources.push_back("Gen_Err");    // handle 0
    m_error_sources.push_back("Gen_Warn");   // handle 1
    m_error_sources.push_back("Gen_Info");   // handle 2

}

rctdlDefaultErrorLogger::~rctdlDefaultErrorLogger()
{
    if(m_created_logger)
        delete m_logger;

    if(m_lastErr)
        delete m_lastErr;

    for(int i = 0; i < 0x80; i++)
        if(m_lastErrID[i] != 0) delete m_lastErrID[i];
}

bool rctdlDefaultErrorLogger::initErrorLogger(const rctdl_err_severity_t verbosity, int opflags)
{
    bool bInit = true;
    rctdlMsgLogger *p_logger = new (std::nothrow) rctdlMsgLogger();
    if(p_logger)
    {
        m_created_logger = true;
        p_logger->setLogOpts(opflags);
        bInit = initErrorLogger(verbosity,p_logger);
    }
    else
    {
        bInit = false;
    }
   return bInit;
}

bool rctdlDefaultErrorLogger::initErrorLogger(const rctdl_err_severity_t verbosity, rctdlMsgLogger *p_msg_logger)
{
    m_Verbosity = verbosity;
    m_logger = p_msg_logger;
    return true;
}

void rctdlDefaultErrorLogger::setLogFilename(const char *name)
{
    if(m_logger)
        m_logger->setLogFileName(name);
}

const rctdl_hndl_err_log_t rctdlDefaultErrorLogger::RegisterErrorSource(const std::string &component_name)
{
    rctdl_hndl_err_log_t handle = m_error_sources.size();
    m_error_sources.push_back(component_name);
    return handle;
}

void rctdlDefaultErrorLogger::LogError(const rctdl_hndl_err_log_t handle, const rctdlError *Error)
{
    // only log errors that match or exceed the current verbosity
    if(m_Verbosity >= Error->getErrorSeverity())
    {
        // print out only if required
        if(m_logger)
        {
            if(m_logger->isLogging())
            {
                std::string errStr = "unknown";
                if(handle < m_error_sources.size())
                    errStr = m_error_sources[handle];
                errStr += " : " + rctdlError::getErrorString(Error);
                m_logger->LogMsg(errStr);
            }
        }

        // log last error
        if(m_lastErr == 0)
            CreateErrorObj(&m_lastErr,Error);
        else
            *m_lastErr = Error;

        // log last error associated with an ID
        if(RCTDL_IS_VALID_CS_SRC_ID(Error->getErrorChanID()))
        {
            if(m_lastErrID[Error->getErrorChanID()] == 0)
                CreateErrorObj(&m_lastErrID[Error->getErrorChanID()], Error);
            else
                *m_lastErrID[Error->getErrorChanID()] = Error;
        }
    }
}

void rctdlDefaultErrorLogger::LogMessage(const rctdl_hndl_err_log_t handle, const rctdl_err_severity_t filter_level, const std::string &msg )
{
    // only log errors that match or exceed the current verbosity
    if((m_Verbosity >= filter_level))
    {
        if(m_logger)
        {
            if(m_logger->isLogging())
            {
                std::string errStr = "unknown";
                if(handle < m_error_sources.size())
                    errStr = m_error_sources[handle];
                errStr += " : " + msg;
                m_logger->LogMsg(errStr);
            }
        }
    }
}

void rctdlDefaultErrorLogger::CreateErrorObj(rctdlError **ppErr, const rctdlError *p_from)
{
    *ppErr = new (std::nothrow) rctdlError(p_from);
}

/* End of File rctdl_error_logger.cpp */

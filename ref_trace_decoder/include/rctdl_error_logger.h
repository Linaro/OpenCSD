/*
 * \file       rctdl_error_logger.h
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

#ifndef ARM_RCTDL_ERROR_LOGGER_H_INCLUDED
#define ARM_RCTDL_ERROR_LOGGER_H_INCLUDED

#include <string>
#include <vector>
#include <fstream>

#include "interfaces/trc_error_log_i.h"
#include "rctdl_error.h"
#include "rctdl_msg_logger.h"

class rctdlDefaultErrorLogger : public ITraceErrorLog
{
public:
    rctdlDefaultErrorLogger();
    virtual ~rctdlDefaultErrorLogger();

    bool initErrorLogger(const rctdl_err_severity_t verbosity, int opflags);
    bool initErrorLogger(const rctdl_err_severity_t verbosity, rctdlMsgLogger *p_msg_logger);
    void setLogFilename(const char *name);           

    virtual const rctdl_hndl_err_log_t RegisterErrorSource(const std::string &component_name);

    virtual void LogError(const rctdl_hndl_err_log_t handle, const rctdlError *Error);
    virtual void LogMessage(const rctdl_hndl_err_log_t handle, const rctdl_err_severity_t filter_level, const std::string &msg );

    virtual const rctdl_err_severity_t GetErrorLogVerbosity() const { return m_Verbosity; };

    virtual rctdlError *GetLastError() { return m_lastErr; };
    virtual rctdlError *GetLastIDError(const uint8_t chan_id)
    { 
        if(RCTDL_IS_VALID_CS_SRC_ID(chan_id))
            return m_lastErrID[chan_id];
        return 0;
    };
    
private:
    void CreateErrorObj(rctdlError **ppErr, const rctdlError *p_from);

    rctdlError *m_lastErr;
    rctdlError *m_lastErrID[0x80];

    rctdl_err_severity_t m_Verbosity;

    rctdlMsgLogger *m_logger;   // pointer to a standard message logger;
    bool m_created_logger;      // true if this class created it's own logger;

    std::vector<std::string> m_error_sources;
};


#endif // ARM_RCTDL_ERROR_LOGGER_H_INCLUDED

/* End of File rctdl_error_logger.h */

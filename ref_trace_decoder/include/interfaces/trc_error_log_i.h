/*
 * \file       trc_error_log_i.h
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

#ifndef ARM_TRC_ERROR_LOG_I_H_INCLUDED
#define ARM_TRC_ERROR_LOG_I_H_INCLUDED

#include "rctdl_if_types.h"
#include <string>

class rctdlError;

/*!
 * @class ITraceErrorLog 
 * @brief Error logging interface. 
 * @ingroup rctdl_interfaces
 * 
 *  This class provides a standard interface to the decoder error logger for all trace decode and 
 *  reader components.
 * 
 */
class ITraceErrorLog
{
public:
    ITraceErrorLog() {};    /**< default constructor */
    virtual ~ITraceErrorLog() {};   /**< default destructor */

    /*!
     * Register a named component error source. Allows the logger to associate errors with components.
     * returned handle to be used with subsequent error log calls.
     *
     * @param &component_name : name of the component.
     *
     * @return virtual const  : Handle associated with the component.
     */
    virtual const rctdl_hndl_err_log_t RegisterErrorSource(const std::string &component_name) = 0;

    /*!
     *  Return the verbosity level of the logger. Errors of the returned rctdl_err_severity_t severity 
     *  or lower will be logged, others are ignored.
     *
     * @return rctdl_err_severity_t  : Current logging severity level.
     */
    virtual const rctdl_err_severity_t GetErrorLogVerbosity() const = 0;

    /*!
     * Log an error. 
     * Pass an error object and the component or generic handle to associate with the error.
     * Error will be saved for access by GetLastError().
     *
     * @param handle : Component handle or standard generic handle
     * @param *Error : Pointer to an error object.
     */
    virtual void LogError(const rctdl_hndl_err_log_t handle, const rctdlError *Error) = 0;

    /*!
     * Log a general message. Associated with component or use generic handle.
     * Message logged to same output as errors, but not saved for GetLastError()
     *
     * @param handle : Component handle or standard generic handle.
     * @param filter_level : Verbosity filter.
     * @param msg    : Pointer to an error object.
     */
    virtual void LogMessage(const rctdl_hndl_err_log_t handle, const rctdl_err_severity_t filter_level, const std::string &msg ) = 0;

    /*!
     * Get a pointer to the last logged error. 
     * Returns 0 if no errors have been logged.
     *
     * @return rctdlError *: last error pointer.
     */
    virtual rctdlError *GetLastError() = 0;

    /*!
     * Get the last error associated with the given Trace source channel ID.
     * returns a pointer to the error or 0 if no errors associated with the ID.
     *
     * @param chan_id : ID.
     *
     * @return rctdlError *: last error pointer for ID or 0.
     */
    virtual rctdlError *GetLastIDError(const uint8_t chan_id) = 0;


    enum generic_handles {
        HANDLE_GEN_ERR = 0,
        HANDLE_GEN_WARN,
        HANDLE_GEN_INFO,
        /* last value in list */
        HANDLE_FIRST_REGISTERED_COMPONENT /**< 1st valid handle value for components registered with logger */
    };
};

#endif // ARM_TRC_ERROR_LOG_I_H_INCLUDED

/* End of File trc_error_log_i.h */

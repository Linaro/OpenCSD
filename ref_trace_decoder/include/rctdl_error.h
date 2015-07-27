/*
 * \file       rctdl_error.h
 * \brief      Reference CoreSight Trace Decoder : Error class 
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

#ifndef ARM_RCTDL_ERROR_H_INCLUDED
#define ARM_RCTDL_ERROR_H_INCLUDED

#include "rctdl_if_types.h"
#include <string>
/** @ingroup rctdl_infrastructure
@{*/

/*!
 * @class rctdlError   
 * 
 *  This class is the error object for the RCTDL. 
 *
 *  Errors are created with a severity (rctdl_err_severity_t) and a standard rctdl_err_t error code.
 *  Errors can optionally be created with a trace index (offset from start of capture buffer), and 
 *  trace CoreSight source channel ID.
 *
 *  A custom error message can be appended to the error.
 *
 *  The rctdlError class contains a static function to output a formatted string representation of an error.
 * 
 */
class rctdlError {
public:
    rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code);    /**< Default error constructor with severity and error code. */
    rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx); /**< Constructor with optional trace index. */
    rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx, const uint8_t chan_id);  /**< Constructor with optional trace index and channel ID. */
    rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const std::string &msg);    /**< Default error constructor with severity and error code - plus message. */
    rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx, const std::string &msg); /**< Constructor with optional trace index - plus message. */
    rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx, const uint8_t chan_id, const std::string &msg);  /**< Constructor with optional trace index and channel ID - plus message. */
   
    rctdlError(const rctdlError *pError);   /**< Copy constructor */
    rctdlError(const rctdlError &Error);    /**< Copy constructor */
    ~rctdlError();  /**< Destructor */

    rctdlError& operator=(const rctdlError *p_err);
    rctdlError& operator=(const rctdlError &err);

    void setMessage(const std::string &msg) { m_err_message = msg; };   /**< Set custom error message */
    const std::string &getMessage() const { return m_err_message; };    /**< Get custom error message */
       
    const rctdl_err_t getErrorCode() const { return m_error_code; };    /**< Get error code. */
    const rctdl_err_severity_t getErrorSeverity() const  { return m_sev; }; /**< Get error severity. */
    const rctdl_trc_index_t getErrorIndex() const { return m_idx; };  /**< Get trace index associated with the error. */
    const uint8_t getErrorChanID() const { return m_chan_ID; }; /**< Get the trace source channel ID associated with the error. */

    static const std::string getErrorString(const rctdlError &error);   /**< Generate a formatted error string for the supplied error. */

private:
    static void appendErrorDetails(std::string &errStr, const rctdlError &error);   /**< build the error string. */
    rctdlError();   /**< Make no parameter default constructor inaccessible. */

    rctdl_err_t m_error_code; /**< Error code for this error */
    rctdl_err_severity_t m_sev;   /**< severity for this error */
    rctdl_trc_index_t m_idx;    /**< Trace buffer index associated with this error (optional) */
    uint8_t m_chan_ID;    /**< trace  source ID associated with this error (optional) */

    std::string m_err_message;  /**< Additional text associated with this error (optional) */
};

inline rctdlError& rctdlError::operator=(const rctdlError *p_err)
{
    this->m_error_code = p_err->getErrorCode();
    this->m_sev = p_err->getErrorSeverity();
    this->m_idx = p_err->getErrorIndex();
    this->m_chan_ID = p_err->getErrorChanID();
    this->m_err_message = p_err->getMessage();
    return *this;
}

inline rctdlError& rctdlError::operator=(const rctdlError &err)
{
    return (*this = &err);
}


/** @}*/

#endif // ARM_RCTDL_ERROR_H_INCLUDED

/* End of File rctdl_error.h */

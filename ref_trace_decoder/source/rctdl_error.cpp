/*
 * \file       rctdl_error.cpp
 * \brief      Reference CoreSight Trace Decoder : Library error class.
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

#include "rctdl_error.h"
#include <sstream>
#include <iomanip>

static const char *s_errorCodeDescs[][2] = {
    /* general return errors */
    {"RCTDL_OK", "No Error."},
    {"RCTDL_ERR_FAIL","General failure."},
    {"RCTDL_ERR_MEM","Internal memory allocation error."},
    {"RCTDL_ERR_NOT_INIT","Component not initialised."},
    {"RCTDL_ERR_INVALID_ID","Invalid CoreSight Trace Source ID."},
    {"RCTDL_ERR_BAD_HANDLE","Invalid handle passed to component."},
    {"RCTDL_ERR_INVALID_PARAM_VAL","Invalid value parameter passed to component."},
    {"RCTDL_ERR_FILE_ERROR","File access error"},
    {"RCTDL_ERR_NO_PROTOCOL","Trace protocol unsupported"},
    /* attachment point errors */
    {"RCTDL_ERR_ATTACH_TOO_MANY","Cannot attach - attach device limit reached."},
    {"RCTDL_ERR_ATTACH_INVALID_PARAM"," Cannot attach - invalid parameter."},
    {"RCTDL_ERR_ATTACH_COMP_NOT_FOUND","Cannot detach - component not found."},
    /* source reader errors */
    {"RCTDL_ERR_RDR_FILE_NOT_FOUND","source reader - file not found."},
    {"RCTDL_ERR_RDR_INVALID_INIT",  "source reader - invalid initialisation parameter."},
    {"RCTDL_ERR_RDR_NO_DECODER", "source reader - not trace decoder set."},
    /* data path errors */
    {"RCTDL_ERR_DATA_DECODE_FATAL", "A decoder in the data path has returned a fatal error."},
    /* frame deformatter errors */
    {"RCTDL_ERR_DFMTR_NOTCONTTRACE", "Trace input to deformatter none-continuous"},
    /* packet processor errors - protocol issues etc */
    {"RCTDL_ERR_BAD_PACKET_SEQ","Bad packet sequence"},
    {"RCTDL_ERR_INVALID_PCKT_HDR","Invalid packet header"},
    /* packet decoder errors */
    {"RCTDL_ERR_UNSUPPORTED_ISA","ISA not supported in decoder"},
    {"RCTDL_ERR_HW_CFG_UNSUPP","Programmed trace configuration not supported by decoder."},
    {"RCTDL_ERR_UNSUPP_DECODE_PKT","Packet not supported in decoder"},
    {"RCTDL_ERR_BAD_DECODE_PKT","Reserved or unknown packet in decoder."}, 
    {"RCTDL_ERR_COMMIT_PKT_OVERRUN","Overrun in commit packet stack - tried to commit more than available"},
    /* decode tree errors */
    {"RCTDL_ERR_DCDT_NO_FORMATTER","No formatter in use - operation not valid."},
    /* target memory access errors */
    {"RCTDL_ERR_MEM_ACC_OVERLAP","Attempted to set an overlapping range in memory access map."},
    {"RCTDL_ERR_MEM_ACC_FILE_NOT_FOUND","Memory access file could not be opened."},
    {"RCTDL_ERR_MEM_ACC_FILE_DIFF_RANGE","Attempt to re-use the same memory access file for a different address range."},
    /* test errors - errors generated only by the test code, not the library */
    {"RCTDL_ERR_TEST_SNAPSHOT_PARSE", "Test snapshot file parse error"},
    {"RCTDL_ERR_TEST_SNAPSHOT_PARSE_INFO", "Test snapshot file parse information"},
    {"RCTDL_ERR_TEST_SNAPSHOT_READ","test snapshot reader error"},
    {"RCTDL_ERR_TEST_SS_TO_DECODER","test snapshot to decode tree conversion error"},
    /* end marker*/
    {"RCTDL_ERR_LAST", "No error - error code end marker"}
};

rctdlError::rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code) :
    m_error_code(code),
    m_sev(sev_type),
    m_idx(RCTDL_BAD_TRC_INDEX),
    m_chan_ID(RCTDL_BAD_CS_SRC_ID)
{
}

rctdlError::rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx) :
    m_error_code(code),
    m_sev(sev_type),
    m_idx(idx),
    m_chan_ID(RCTDL_BAD_CS_SRC_ID)
{
}

rctdlError::rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx, const uint8_t chan_id) :
    m_error_code(code),
    m_sev(sev_type),
    m_idx(idx),
    m_chan_ID(chan_id)
{
}

rctdlError::rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const std::string &msg) :
    m_error_code(code),
    m_sev(sev_type),
    m_idx(RCTDL_BAD_TRC_INDEX),
    m_chan_ID(RCTDL_BAD_CS_SRC_ID),
    m_err_message(msg)
{
}

rctdlError::rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx, const std::string &msg) :
    m_error_code(code),
    m_sev(sev_type),
    m_idx(idx),
    m_chan_ID(RCTDL_BAD_CS_SRC_ID),
    m_err_message(msg)
{
}

rctdlError::rctdlError(const rctdl_err_severity_t sev_type, const rctdl_err_t code, const rctdl_trc_index_t idx, const uint8_t chan_id, const std::string &msg) :
    m_error_code(code),
    m_sev(sev_type),
    m_idx(idx),
    m_chan_ID(chan_id),
    m_err_message(msg)
{
}


rctdlError::rctdlError(const rctdlError *pError) :
    m_error_code(pError->getErrorCode()),
    m_sev(pError->getErrorSeverity()),
    m_idx(pError->getErrorIndex()),
    m_chan_ID(pError->getErrorChanID())
{
    setMessage(pError->getMessage());
}

rctdlError::rctdlError(const rctdlError &Error) :
    m_error_code(Error.getErrorCode()),
    m_sev(Error.getErrorSeverity()),
    m_idx(Error.getErrorIndex()),
    m_chan_ID(Error.getErrorChanID())
{
    setMessage(Error.getMessage());
}

rctdlError::rctdlError():
    m_error_code(RCTDL_ERR_LAST),
    m_sev(RCTDL_ERR_SEV_NONE),
    m_idx(RCTDL_BAD_TRC_INDEX),
    m_chan_ID(RCTDL_BAD_CS_SRC_ID)
{
}

rctdlError::~rctdlError()
{
}

const std::string rctdlError::getErrorString(const rctdlError &error)
{
    std::string szErrStr = "LIBRARY INTERNAL ERROR: Invalid Error Object";
    const char *sev_type_sz[] = {
        "NONE ",
        "ERROR:",
        "WARN :", 
        "INFO :"
    };

    switch(error.getErrorSeverity())
    {
    default:
    case RCTDL_ERR_SEV_NONE:
        break;

    case RCTDL_ERR_SEV_ERROR:
    case RCTDL_ERR_SEV_WARN:
    case RCTDL_ERR_SEV_INFO:
        szErrStr = sev_type_sz[(int)error.getErrorSeverity()];
        appendErrorDetails(szErrStr,error);
        break;
    }
    return szErrStr;
}

void rctdlError::appendErrorDetails(std::string &errStr, const rctdlError &error)
{
    int numerrstr = ((sizeof(s_errorCodeDescs) / sizeof(const char *)) / 2);
    int code = (int)error.getErrorCode();
    rctdl_trc_index_t idx = error.getErrorIndex();
    uint8_t chan_ID = error.getErrorChanID();
    std::ostringstream oss;

    oss << "0x" << std::hex << std::setfill('0') << std::setw(4) << code;
    if(code < numerrstr)
        oss << " (" << s_errorCodeDescs[code][0] << ") [" << s_errorCodeDescs[code][1] << "]; ";
    else
        oss << " (unknown); ";

    if(idx != RCTDL_BAD_TRC_INDEX)
        oss << "TrcIdx=" << std::dec << idx << "; ";

    if(chan_ID != RCTDL_BAD_CS_SRC_ID)
        oss << "CS ID=" << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)chan_ID << "; ";

    oss << error.getMessage();
    errStr = oss.str();
}

/* End of File rctdl_error.cpp */

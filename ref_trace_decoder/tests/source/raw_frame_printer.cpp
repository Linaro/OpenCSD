/*
 * \file       raw_frame_printer.cpp
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

#include <string>
#include <sstream>
#include <iomanip>

#include "raw_frame_printer.h"


rctdl_err_t RawFramePrinter::TraceRawFrameIn(  const rctdl_datapath_op_t op, 
                                                const rctdl_trc_index_t index, 
                                                const rctdl_rawframe_elem_t frame_element, 
                                                const int dataBlockSize, 
                                                const uint8_t *pDataBlock,
                                                const uint8_t traceID)
{

    if(op == RCTDL_OP_DATA) // only interested in actual frame data.
    {
        std::string strData;
        std::ostringstream oss;
        int printDataSize = dataBlockSize;
        int printOffset = 0;

        oss << "Frame Data; Index" << std::setw(7) << index << "; ";
        switch(frame_element) 
        {
        case RCTDL_FRM_PACKED: oss << std::setw(15) << "RAW_PACKED; "; break;
        case RCTDL_FRM_HSYNC:  oss << std::setw(15) << "HSYNC; "; break;
        case RCTDL_FRM_FSYNC:  oss << std::setw(15)  << "FSYNC; "; break;  
        case RCTDL_FRM_ID_DATA: oss << std::setw(10) << "ID_DATA[0x" << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)traceID << "]; "; break;
        default: oss << std::setw(15) << "UNKNOWN; "; break;
        }

        if(printDataSize)
        {
            createDataString(printDataSize,pDataBlock,16,strData);
            oss << strData;
        }
        oss << std::endl;
        itemPrintLine(oss.str());
    }
    return RCTDL_OK;
}

void RawFramePrinter::createDataString(const int dataSize, const uint8_t *pData, int bytesPerLine, std::string &dataStr)
{
    int lineBytes = 0;
    std::ostringstream oss;

    for(int i = 0; i < dataSize; i++)
    {
        if(lineBytes == bytesPerLine)
        {
            oss << std::endl;
            lineBytes = 0;
        }
        oss << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)pData[i] << " ";
        lineBytes ++;
    }
    dataStr = oss.str();
}


/* End of File raw_frame_printer.cpp */

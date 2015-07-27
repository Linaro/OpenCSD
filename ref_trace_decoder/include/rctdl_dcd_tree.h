/*
 * \file       rctdl_dcd_tree.h
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

#ifndef ARM_RCTDL_DCD_TREE_H_INCLUDED
#define ARM_RCTDL_DCD_TREE_H_INCLUDED

#include <vector>

#include "rctdl.h"


/** @defgroup dcd_tree Reference CoreSight Trace Decoder Library : Trace Decode Tree.
    @brief Create a multi source decode tree for a single trace capture buffer.

    Use to creates a connected set of decoder objects to decode a trace buffer.
    There may be multiple trace sources within the capture buffer.

@{*/

#include "rctdl_dcd_tree_elem.h"

class DecodeTree : public ITrcDataIn
{
public:
    DecodeTree();
    ~DecodeTree();

    static DecodeTree *CreateDecodeTree(const rctdl_dcd_tree_src_t src_type, const uint32_t formatterCfgFlags);
    static void DestroyDecodeTree(DecodeTree *p_dcd_tree);

    static rctdlDefaultErrorLogger& getDefaultErrorLogger() { return s_error_logger; };
    static ITraceErrorLog *getCurrentErrorLogI() { return s_i_error_logger; };
    static void setAlternateErrorLogger(ITraceErrorLog *p_error_logger);

    /** decode tree implements the data in interface */
    virtual rctdl_datapath_resp_t TraceDataIn( const rctdl_datapath_op_t op,
                                               const rctdl_trc_index_t index,
                                               const uint32_t dataBlockSize,
                                               const uint8_t *pDataBlock,
                                               uint32_t *numBytesProcessed);



    /* create packet processing element only - attach to CSID in config */
    rctdl_err_t createETMv3PktProcessor(const EtmV3Config *p_config);
    rctdl_err_t createETMv4PktProcessor(const EtmV4Config *p_config, bool bDataChannel = false);
    rctdl_err_t createPTMPktProcessor(const PtmConfig *p_config);

    /* create full decoder - packet processor + packet decoder  - attach to CSID in config */
    rctdl_err_t createETMv3Decoder(const EtmV3Config *p_config);
    rctdl_err_t createETMv4Decoder(const EtmV4Config *p_config, bool bDataChannel = false);
    rctdl_err_t createPTMDecoder(const PtmConfig *p_config);

    /* remove a decoder / packet processor attached to an ID  - allows another decoder to be substituted. */
    rctdl_err_t removeDecoder(const uint8_t CSID);

    /* attach custom / external decoders  */
    rctdl_err_t  attachExternPktProcessor(const uint8_t CSID, ITrcDataIn* ext_data_in, void *p_pkt_proc_obj);
    rctdl_err_t  attachExternDecoder(const uint8_t CSID, ITrcDataIn* ext_data_in, void *p_pkt_proc_obj, void *p_decoder_obj);

    /* set key interfaces - attach / replace on any existing tree components? */
    void setInstrDecoder(IInstrDecode *i_instr_decode);
    void setMemAccessor(ITargetMemAccess *i_mem_access);
    void setGenTraceElemOutI(ITrcGenElemIn *i_gen_trace_elem);

    /* get decoder elements currently in use  */
    DecodeTreeElement *getDecoderElement(const uint8_t CSID) const;
    /* iterate decoder elements */ 
    DecodeTreeElement *getFirstElement(uint8_t &elemID);
    DecodeTreeElement *getNextElement(uint8_t &elemID);

    TraceFormatterFrameDecoder *getFrameDeformatter() const { return m_frame_deformatter_root; };
    ITrcGenElemIn *getGenTraceElemOutI() const { return m_i_gen_elem_out; };

private:
    bool initialise(const rctdl_dcd_tree_src_t type, uint32_t formatterCfgFlags);
    const bool usingFormatter() const { return (bool)(m_dcd_tree_type ==  RCTDL_TRC_SRC_FRAME_FORMATTED); };
    void setSingleRoot(TrcPktProcI *pComp);
    rctdl_err_t createDecodeElement(const uint8_t CSID);
    void destroyDecodeElement(const uint8_t CSID);

    rctdl_dcd_tree_src_t m_dcd_tree_type;

    IInstrDecode *m_i_instr_decode;
    ITargetMemAccess *m_i_mem_access;
    ITrcGenElemIn *m_i_gen_elem_out;    //!< Output interface for generic elements from decoder.

    ITrcDataIn* m_i_decoder_root;   /*!< root decoder object interface - either deformatter or single packet processor */

    TraceFormatterFrameDecoder *m_frame_deformatter_root;

    DecodeTreeElement *m_decode_elements[0x80];

    uint8_t m_decode_elem_iter;

    /* global error logger  - all sources */ 
    static ITraceErrorLog *s_i_error_logger;
    static std::vector<DecodeTree *> s_trace_dcd_trees;
    static rctdlDefaultErrorLogger s_error_logger;
};

#endif // ARM_RCTDL_DCD_TREE_H_INCLUDED

/* End of File rctdl_dcd_tree.h */

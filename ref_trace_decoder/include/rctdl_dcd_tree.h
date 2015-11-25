/*!
 * \file       rctdl_dcd_tree.h
 * \brief      Reference CoreSight Trace Decoder : Trace Decode Tree.
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
#include <list>

#include "rctdl.h"


/** @defgroup dcd_tree Reference CoreSight Trace Decoder Library : Trace Decode Tree.
    @brief Create a multi source decode tree for a single trace capture buffer.

    Use to create a connected set of decoder objects to decode a trace buffer.
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

    /** The library default error logger */
    static rctdlDefaultErrorLogger* getDefaultErrorLogger() { return &s_error_logger; };

    /** error logging interface in use */
    static ITraceErrorLog *getCurrentErrorLogI() { return s_i_error_logger; };
    static void setAlternateErrorLogger(ITraceErrorLog *p_error_logger);

    /** decode tree implements the data in interface */
    virtual rctdl_datapath_resp_t TraceDataIn( const rctdl_datapath_op_t op,
                                               const rctdl_trc_index_t index,
                                               const uint32_t dataBlockSize,
                                               const uint8_t *pDataBlock,
                                               uint32_t *numBytesProcessed);



    /* create packet processing element only - attach to CSID in config
       optionally attach the output interface
       */
    rctdl_err_t createETMv3PktProcessor(EtmV3Config *p_config, IPktDataIn<EtmV3TrcPacket> *p_Iout = 0);
    rctdl_err_t createETMv4IPktProcessor(EtmV4Config *p_config, IPktDataIn<EtmV4ITrcPacket> *p_Iout = 0);
    rctdl_err_t createETMv4DPktProcessor(EtmV4Config *p_config, IPktDataIn<EtmV4DTrcPacket> *p_Iout = 0);
    rctdl_err_t createPTMPktProcessor(PtmConfig *p_config, IPktDataIn<PtmTrcPacket> *p_Iout = 0);
    rctdl_err_t createSTMPktProcessor(STMConfig *p_config, IPktDataIn<StmTrcPacket> *p_Iout = 0);

    /* create full decoder - packet processor + packet decoder  - attach to CSID in config 
       All use the standard generic elements output interface.
    */
    rctdl_err_t createETMv3Decoder(EtmV3Config *p_config);
    rctdl_err_t createETMv4Decoder(EtmV4Config *p_config, bool bDataChannel = false);
    rctdl_err_t createPTMDecoder(PtmConfig *p_config);

    /* remove a decoder / packet processor attached to an ID  - allows another decoder to be substituted. */
    rctdl_err_t removeDecoder(const uint8_t CSID);

    /* attach custom / external decoders  */
    rctdl_err_t  attachExternPktProcessor(const uint8_t CSID, ITrcDataIn* ext_data_in, void *p_pkt_proc_obj);
    rctdl_err_t  attachExternDecoder(const uint8_t CSID, ITrcDataIn* ext_data_in, void *p_pkt_proc_obj, void *p_decoder_obj);

    /* set key interfaces - attach / replace on any existing tree components? */
    void setInstrDecoder(IInstrDecode *i_instr_decode);
    void setMemAccessI(ITargetMemAccess *i_mem_access);
    void setGenTraceElemOutI(ITrcGenElemIn *i_gen_trace_elem);

    /* create mapper within the decode tree. */ 
    rctdl_err_t createMemAccMapper(memacc_mapper_t type = MEMACC_MAP_GLOBAL);
    rctdl_err_t addMemAccessorToMap(TrcMemAccessorBase *p_accessor, const uint8_t cs_trace_id);
    const bool hasMemAccMapper() const { return (bool)(m_default_mapper != 0); };

    /* get decoder elements currently in use  */
    DecodeTreeElement *getDecoderElement(const uint8_t CSID) const;
    /* iterate decoder elements */ 
    DecodeTreeElement *getFirstElement(uint8_t &elemID);
    DecodeTreeElement *getNextElement(uint8_t &elemID);

    TraceFormatterFrameDecoder *getFrameDeformatter() const { return m_frame_deformatter_root; };
    ITrcGenElemIn *getGenTraceElemOutI() const { return m_i_gen_elem_out; };

    /* ID filtering - sets the output filter on the trace deformatter. No effect if no decoder attached for the ID */
    rctdl_err_t setIDFilter(std::vector<uint8_t> &ids);  // only supplied IDs will be decoded
    rctdl_err_t clearIDFilter(); // remove filter, all IDs will be decoded

private:
    bool initialise(const rctdl_dcd_tree_src_t type, uint32_t formatterCfgFlags);
    const bool usingFormatter() const { return (bool)(m_dcd_tree_type ==  RCTDL_TRC_SRC_FRAME_FORMATTED); };
    void setSingleRoot(TrcPktProcI *pComp);
    rctdl_err_t createDecodeElement(const uint8_t CSID);
    void destroyDecodeElement(const uint8_t CSID);
    void destroyMemAccMapper();

    rctdl_dcd_tree_src_t m_dcd_tree_type;

    IInstrDecode *m_i_instr_decode;
    ITargetMemAccess *m_i_mem_access;
    ITrcGenElemIn *m_i_gen_elem_out;    //!< Output interface for generic elements from decoder.

    ITrcDataIn* m_i_decoder_root;   /*!< root decoder object interface - either deformatter or single packet processor */

    TraceFormatterFrameDecoder *m_frame_deformatter_root;

    DecodeTreeElement *m_decode_elements[0x80];

    uint8_t m_decode_elem_iter;

    TrcMemAccMapper *m_default_mapper;

    /* global error logger  - all sources */ 
    static ITraceErrorLog *s_i_error_logger;
    static std::list<DecodeTree *> s_trace_dcd_trees;

    /**! default error logger */
    static rctdlDefaultErrorLogger s_error_logger;

    /**! default instruction decoder */
    static TrcIDecode s_instruction_decoder;
};

/** @}*/

#endif // ARM_RCTDL_DCD_TREE_H_INCLUDED

/* End of File rctdl_dcd_tree.h */

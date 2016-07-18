/*!
 * \file       ocsd_dcd_tree.h
 * \brief      OpenCSD : Trace Decode Tree.
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

#ifndef ARM_OCSD_DCD_TREE_H_INCLUDED
#define ARM_OCSD_DCD_TREE_H_INCLUDED

#include <vector>
#include <list>

#include "opencsd.h"


/** @defgroup dcd_tree OpenCSD Library : Trace Decode Tree.
    @brief Create a multi source decode tree for a single trace capture buffer.

    Use to create a connected set of decoder objects to decode a trace buffer.
    There may be multiple trace sources within the capture buffer.

@{*/

#include "ocsd_dcd_tree_elem.h"

/*!
 * @class DecodeTree
 * @brief Class to manage the decoding of data from a single trace sink .
 * 
 *  Provides functionality to build a tree of decode objects capable of decoding 
 *  multiple trace sources within a single trace sink (capture buffer).
 * 
 */
class DecodeTree : public ITrcDataIn
{
public:
    DecodeTree();
    ~DecodeTree();

    /*!
     * Create a decode tree.
     * Automatically creates a trace frame deformatter if required and a default error log component.
     *
     * @param src_type : Data stream source type, can be CoreSight frame formatted trace, or single demuxed trace data stream,
     * @param formatterCfgFlags : Configuration flags for trace de-formatter.
     *
     * @return DecodeTree * : pointer to the decode tree, 0 if creation failed.
     */
    static DecodeTree *CreateDecodeTree(const ocsd_dcd_tree_src_t src_type, const uint32_t formatterCfgFlags);

    /** Destroy a decode tree */
    static void DestroyDecodeTree(DecodeTree *p_dcd_tree);

    /** The library default error logger */
    static ocsdDefaultErrorLogger* getDefaultErrorLogger() { return &s_error_logger; };

    /** the current error logging interface in use */
    static ITraceErrorLog *getCurrentErrorLogI() { return s_i_error_logger; };

    /** set an alternate error logging interface. */
    static void setAlternateErrorLogger(ITraceErrorLog *p_error_logger);

    /** decode tree implements the data in interface : ITrcDataIn .
        Captured trace data is passed to the deformatter and decoders via this method.
    */
    virtual ocsd_datapath_resp_t TraceDataIn( const ocsd_datapath_op_t op,
                                               const ocsd_trc_index_t index,
                                               const uint32_t dataBlockSize,
                                               const uint8_t *pDataBlock,
                                               uint32_t *numBytesProcessed);

    /*! Create a decoder by registered name */
    ocsd_err_t createDecoder(const std::string &decoderName, const int createFlags, const CSConfig *pConfig);

    /* remove a decoder / packet processor attached to an ID  - allows another decoder to be substituted. */
    ocsd_err_t removeDecoder(const uint8_t CSID);

    /* set key interfaces - attach / replace on any existing tree components? */
    void setInstrDecoder(IInstrDecode *i_instr_decode);
    void setMemAccessI(ITargetMemAccess *i_mem_access);
    void setGenTraceElemOutI(ITrcGenElemIn *i_gen_trace_elem);

    /* create mapper within the decode tree - also allows direct manipulation of the mapper object to set up custom arrangements of accessors. */ 
    ocsd_err_t createMemAccMapper(memacc_mapper_t type = MEMACC_MAP_GLOBAL);
    TrcMemAccMapper *getMemAccMapper() const { return m_default_mapper; };
    void setExternMemAccMapper(TrcMemAccMapper * pMapper);
    const bool hasMemAccMapper() const { return (bool)(m_default_mapper != 0); };
    void logMappedRanges();

    /*  create and destroy accessor types - all using global CSID value - on default accessor */
    ocsd_err_t addBufferMemAcc(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const uint8_t *p_mem_buffer, const uint32_t mem_length);
    ocsd_err_t addBinFileMemAcc(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space, const std::string &filepath);
    ocsd_err_t addBinFileRegionMemAcc(const ocsd_file_mem_region_t *region_array, const int num_regions, const ocsd_mem_space_acc_t mem_space, const std::string &filepath);
    ocsd_err_t addCallbackMemAcc(const ocsd_vaddr_t st_address, const ocsd_vaddr_t en_address, const ocsd_mem_space_acc_t mem_space, Fn_MemAcc_CB p_cb_func, const void *p_context); 
    ocsd_err_t removeMemAccByAddress(const ocsd_vaddr_t address, const ocsd_mem_space_acc_t mem_space);

    /* get decoder elements currently in use  */
    DecodeTreeElement *getDecoderElement(const uint8_t CSID) const;
    /* iterate decoder elements */ 
    DecodeTreeElement *getFirstElement(uint8_t &elemID);
    DecodeTreeElement *getNextElement(uint8_t &elemID);

    TraceFormatterFrameDecoder *getFrameDeformatter() const { return m_frame_deformatter_root; };
    ITrcGenElemIn *getGenTraceElemOutI() const { return m_i_gen_elem_out; };

    /* ID filtering - sets the output filter on the trace deformatter. No effect if no decoder attached for the ID */
    ocsd_err_t setIDFilter(std::vector<uint8_t> &ids);  // only supplied IDs will be decoded
    ocsd_err_t clearIDFilter(); // remove filter, all IDs will be decoded

private:
    bool initialise(const ocsd_dcd_tree_src_t type, uint32_t formatterCfgFlags);
    const bool usingFormatter() const { return (bool)(m_dcd_tree_type ==  OCSD_TRC_SRC_FRAME_FORMATTED); };
    void setSingleRoot(TrcPktProcI *pComp);
    ocsd_err_t createDecodeElement(const uint8_t CSID);
    void destroyDecodeElement(const uint8_t CSID);
    void destroyMemAccMapper();

    ocsd_dcd_tree_src_t m_dcd_tree_type;

    IInstrDecode *m_i_instr_decode;
    ITargetMemAccess *m_i_mem_access;
    ITrcGenElemIn *m_i_gen_elem_out;    //!< Output interface for generic elements from decoder.

    ITrcDataIn* m_i_decoder_root;   /*!< root decoder object interface - either deformatter or single packet processor */

    TraceFormatterFrameDecoder *m_frame_deformatter_root;

    DecodeTreeElement *m_decode_elements[0x80];

    uint8_t m_decode_elem_iter;

    TrcMemAccMapper *m_default_mapper;  //!< the mem acc mapper to use
    bool m_created_mapper;              //!< true if created by decode tree object

    /* global error logger  - all sources */ 
    static ITraceErrorLog *s_i_error_logger;
    static std::list<DecodeTree *> s_trace_dcd_trees;

    /**! default error logger */
    static ocsdDefaultErrorLogger s_error_logger;

    /**! default instruction decoder */
    static TrcIDecode s_instruction_decoder;
};

/** @}*/

#endif // ARM_OCSD_DCD_TREE_H_INCLUDED

/* End of File ocsd_dcd_tree.h */

/*
 * \file       rctdl_c_api_obj.h
 * \brief      Reference CoreSight Trace Decoder : 
 * 
 * \copyright  Copyright (c) 2015, ARM Limited. All Rights Reserved.
 */

#ifndef ARM_RCTDL_C_API_OBJ_H_INCLUDED
#define ARM_RCTDL_C_API_OBJ_H_INCLUDED

#include "c_api/rctdl_c_api_types.h"
#include "interfaces/trc_gen_elem_in_i.h"

class GenTraceElemCBObj : public ITrcGenElemIn
{
public:
    GenTraceElemCBObj(FnTraceElemIn pCBFn);
    virtual ~GenTraceElemCBObj() {};

    virtual rctdl_datapath_resp_t TraceElemIn(const rctdl_trc_index_t index_sop,
                                              const uint8_t trc_chan_id,
                                              const RctdlTraceElement &elem);

private:
    FnTraceElemIn m_c_api_cb_fn;
};




#endif // ARM_RCTDL_C_API_OBJ_H_INCLUDED

/* End of File rctdl_c_api_obj.h */

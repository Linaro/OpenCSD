/*
 * \file       trc_etmv4_stack_elem.h
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
#ifndef ARM_TRC_ETMV4_STACK_ELEM_H_INCLUDED
#define ARM_TRC_ETMV4_STACK_ELEM_H_INCLUDED

#include "etmv4/trc_pkt_types_etmv4.h"




// decoder must maintain stack of last 3 broadcast addresses.
// used in "same address" packets.
class AddrValStack {
public:
    AddrValStack() {};
    ~AddrValStack() {};

    void push(const etmv4_addr_val_t &val);

    const etmv4_addr_val_t &get(const int N) const { return m_stack[N & 0x3]; };

private:

    etmv4_addr_val_t m_stack[4];  // only need 3, but 4th allows easy limit checking for N
};

inline void AddrValStack::push(const etmv4_addr_val_t &val)
{
    m_stack[2] = m_stack[1];
    m_stack[1] = m_stack[0];
    m_stack[0] = val;
}

/* ETMv4 I trace stack elements  
    Speculation requires that we stack certain elements till they are committed or 
    cancelled. (P0 elements + other associated parts.)
*/

typedef enum _p0_elem_t 
{
    P0_UNKNOWN,
    P0_ATOM,
    P0_ADDR,
    P0_CTXT,
    P0_TRC_ON,
    P0_EXCEP,
    P0_EXCEP_RET,
    P0_EVENT,
    P0_TS,
    P0_CC,
    P0_TS_CC,
    P0_OVERFLOW
} p0_elem_t;


/************************************************************/
/***Trace stack element base class - 
    record originating packet type and index in buffer*/ 

class TrcStackElem {
public:
     TrcStackElem(const p0_elem_t p0_type, const bool isP0, const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index);
     virtual ~TrcStackElem() {};

     const p0_elem_t getP0Type() const { return m_P0_type; };
     const rctdl_etmv4_i_pkt_type getRootPkt() const { return m_root_pkt; };
     const rctdl_trc_index_t getRootIndex() const  { return m_root_idx; };
     const bool isP0() const { return m_is_P0; };

private:
     rctdl_etmv4_i_pkt_type m_root_pkt;
     rctdl_trc_index_t m_root_idx;
     p0_elem_t m_P0_type;

protected:
     bool m_is_P0;  // true if genuine P0 - commit / cancellable, false otherwise

};

inline TrcStackElem::TrcStackElem(p0_elem_t p0_type, const bool isP0, rctdl_etmv4_i_pkt_type root_pkt, rctdl_trc_index_t root_index) :
    m_root_pkt(root_pkt),
    m_root_idx(root_index),
    m_P0_type(p0_type),
    m_is_P0(isP0)
{
}

/************************************************************/
/** Address element */

class TrcStackElemAddr : public TrcStackElem
{
public:
    TrcStackElemAddr(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index);
    virtual ~TrcStackElemAddr() {};

    void setAddr(const etmv4_addr_val_t addr_val, const bool is64bit) { m_addr_val = addr_val; m_64bit = is64bit; };
    void setAddrMatch(const int idx) { m_is_addr_match = true; m_addr_match_idx = idx; };

    const bool is64bit() const { return m_64bit; };
    const etmv4_addr_val_t &getAddr() const { return m_addr_val; };
    const bool isAddrMatch(int &idx) const { idx = m_addr_match_idx; return m_is_addr_match; };


private:
    etmv4_addr_val_t m_addr_val;
    bool m_64bit;
    bool m_is_addr_match;
    int  m_addr_match_idx;
};

inline TrcStackElemAddr::TrcStackElemAddr(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index) :
    TrcStackElem(P0_ADDR, false, root_pkt,root_index),
        m_64bit(false),
        m_is_addr_match(false),
        m_addr_match_idx(0)
{
    m_addr_val.val = 0;
    m_addr_val.isa = 0;
}

/************************************************************/
/** Context element */
    
class TrcStackElemCtxt : public TrcStackElem
{
public:
    TrcStackElemCtxt(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index);
    virtual ~TrcStackElemCtxt() {};

    void setContext(const  etmv4_context_t &ctxt) { m_context = ctxt; };
    const  etmv4_context_t &getContext() const  { return m_context; }; 

private:
     etmv4_context_t m_context;
};

inline TrcStackElemCtxt::TrcStackElemCtxt(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index) :
    TrcStackElem(P0_CTXT, false, root_pkt,root_index)
{
}

/************************************************************/
/** Exception element */

class TrcStackElemExcept : public TrcStackElem
{
public:
    TrcStackElemExcept(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index);
    virtual ~TrcStackElemExcept() {};

    void setPrevSame(bool bSame) { m_prev_addr_same = bSame; };
    const bool getPrevSame() const { return m_prev_addr_same; };

    void setExcepNum(const uint16_t num) { m_excep_num = num; };
    const uint16_t getExcepNum() const { return m_excep_num; };

private:
    bool m_prev_addr_same;
    uint16_t m_excep_num;
};

inline TrcStackElemExcept::TrcStackElemExcept(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index) :
    TrcStackElem(P0_EXCEP, true, root_pkt,root_index),
        m_prev_addr_same(false)
{
}

/************************************************************/
/** Atom element */
    
class TrcStackElemAtom : public TrcStackElem
{
public:
    TrcStackElemAtom(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index);
    virtual ~TrcStackElemAtom() {};

    void setAtom(const rctdl_pkt_atom &atom) { m_atom = atom; };

    const rctdl_atm_val commitOldest();
    int cancelNewest(const int nCancel);
    const bool isEmpty() const { return (m_atom.num == 0); };

private:
    rctdl_pkt_atom m_atom;
};

inline TrcStackElemAtom::TrcStackElemAtom(const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index) :
    TrcStackElem(P0_ATOM, true, root_pkt,root_index)
{
    m_atom.num = 0;
}

// commit oldest - get value and remove it from pattern
inline const rctdl_atm_val TrcStackElemAtom::commitOldest()
{
    rctdl_atm_val val = (m_atom.En_bits & 0x1) ? ATOM_E : ATOM_N;
    m_atom.num--;
    m_atom.En_bits >>= 1;
    return val;
}

// cancel newest - just reduce the atom count.
inline int TrcStackElemAtom::cancelNewest(const int nCancel)
{
    int nRemove = (nCancel <= m_atom.num) ? nCancel : m_atom.num;
    m_atom.num -= nRemove;
    return nRemove;
}

/************************************************************/
/** Generic param element */

class TrcStackElemParam : public TrcStackElem
{
public:
    TrcStackElemParam(const p0_elem_t p0_type, const bool isP0, const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index);
    virtual ~TrcStackElemParam() {};

    void setParam(const uint32_t param, const int nParamNum) { m_param[(nParamNum & 0x3)] = param; };
    const uint32_t &getParam(const int nParamNum) const { return m_param[(nParamNum & 0x3)]; };

private:
    uint32_t m_param[4];    
};

inline TrcStackElemParam::TrcStackElemParam(const p0_elem_t p0_type, const bool isP0, const rctdl_etmv4_i_pkt_type root_pkt, const rctdl_trc_index_t root_index) :
    TrcStackElem(p0_type, isP0, root_pkt,root_index)
{
}

#endif // ARM_TRC_ETMV4_STACK_ELEM_H_INCLUDED

/* End of File trc_etmv4_stack_elem.h */

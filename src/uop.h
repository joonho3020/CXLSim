/*
Copyright (c) <2012>, <Seoul National University> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials provided
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors
may be used to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/**********************************************************************************************
 * File         : uop.h
 * Author       : Joonho
 * Date         : 02/5/2022
 * SVN          : $Id: uop.h,v 1.5 2021-10-08 21:01:41 kacear Exp $:
 * Description  : NDP uop
 *********************************************************************************************/

#ifndef UOP_H
#define UOP_H

#include "cxlsim.h"
#include "global_defs.h"
#include "global_types.h"

namespace cxlsim {

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief uop types
///////////////////////////////////////////////////////////////////////////////////////////////
typedef enum Uop_Type_enum {
  UOP_INV,  //!< invalid opcode
  UOP_SPEC,  //!< something weird (rpcc)

  // UOP_SSMT0,   // something added for ssmt uarch interaction
  // UOP_SSMT1,   // something added for ssmt uarch interaction
  // UOP_SSMT2,   // something added for ssmt uarch interaction
  // UOP_SSMT3,   // something added for ssmt uarch interaction

  UOP_NOP,  //!< is a decoded nop

  // these instructions use all integer regs
  UOP_CF,  //!< change of flow
  UOP_CMOV,  //!< conditional move
  UOP_LDA,  //!< load address
  UOP_IMEM,  //!< int memory instruction
  UOP_IADD,  //!< integer add
  UOP_IMUL,  //!< integer multiply
  UOP_IDIV,  //!< integer divide
  UOP_ICMP,  //!< integer compare
  UOP_LOGIC,  //!< logical
  UOP_SHIFT,  //!< shift
  UOP_BYTE,  //!< byte manipulation
  UOP_MM,  //!< multimedia instructions

  // vector instructions
  UOP_VADD,  //!< vector integer add
  UOP_VSTR,  //!< vector string operation
  UOP_VFADD,  //!< vector floating point add

  // fence instruction
  UOP_LFENCE,  //!< load fence, TODO
  UOP_FULL_FENCE,  //!< Full memory fence
  UOP_ACQ_FENCE,  //!< acquire fence
  UOP_REL_FENCE,  //!< release fence

  // fmem reads one int reg and writes a fp reg
  UOP_FMEM,  //!< fp memory instruction

  // everything below here is floating point regs only
  UOP_FCF,
  UOP_FCVT,  //!< floating point convert
  UOP_FADD,  //!< floating point add
  UOP_FMUL,  //!< floating point multiply
  UOP_FDIV,  //!< floating point divide
  UOP_FCMP,  //!< floating point compare
  UOP_FBIT,  //!< floating point bit
  UOP_FCMOV,  //!< floating point cond move

  UOP_LD,  //!< load memory instruction
  UOP_ST,  //!< store memory instruction

  // MMX instructions
  UOP_SSE,

  // SIMD instructions for Intel GPU
  UOP_SIMD,

  // other instructions
  UOP_AES,  //!< AES enctyption
  UOP_PCLMUL,  //!< carryless multiplication
  UOP_X87,  //!< x87 ALU op
  UOP_XSAVE,  //!< XSAVE context switch
  UOP_XSAVEOPT,  //!< optimized XSAVE context switch

  NUM_UOP_TYPES,
} Uop_Type;

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief  Mem_Type breaks down the different types of memory operations into
/// loads, stores, and prefetches.
///////////////////////////////////////////////////////////////////////////////////////////////
typedef enum Mem_Type_enum {
  NOT_MEM,  //!< not a memory instruction
  MEM_LD,  //!< a load instruction
  MEM_ST,  //!< a store instruction
  MEM_PF,  //!< a prefetch
  MEM_WH,  //!< a write hint
  MEM_EVICT,  //!< a cache block eviction hint
  MEM_SWPREF_NTA,
  MEM_SWPREF_T0,
  MEM_SWPREF_T1,
  MEM_SWPREF_T2,
  NUM_MEM_TYPES,
} Mem_Type;

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Dependence type
///////////////////////////////////////////////////////////////////////////////////////////////
typedef enum Dep_Type_enum {
  REG_DATA_DEP,
  MEM_ADDR_DEP,
  MEM_DATA_DEP,
  PREV_UOP_DEP,
  NUM_DEP_TYPES,
} Dep_Type;

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Source uop information class
///////////////////////////////////////////////////////////////////////////////////////////////
typedef struct src_info_s {
  Dep_Type m_type; /**< dependence type */
  uop_s *m_uop; /**< uop pointer */
} src_info_s;

typedef struct uop_s {
  /** 
   * Constructor 
   */
  uop_s(cxlsim_c* simBase);
  void init(void);
  bool is_write(void);
  void print(void);

  Counter m_unique_num; /**< unique uop number */
  Uop_Type m_uop_type; /**< uop type */
  Mem_Type m_mem_type; /**< memory access type */

  bool m_valid; /**< source uop is valid */

  Addr m_addr; /**< memory access address */
  int m_latency; /**< latency of uop */

  Counter m_last_dep_exec; /**< cycle when last dep uop is done */
  Counter m_exec_cycle; /**< cycle when the uop was executed */
  Counter m_done_cycle; /**< cycle when uop execution is finished */

  bool m_src_rdy; /**< src uops ready */
  int m_src_cnt; /**< src uop count */
  src_info_s m_map_src_info[MAX_UOP_SRC_DEPS]; /** src uop info */
  cxlsim_c* m_simBase;
} uop_s;

} // namespace CXL

#endif //UOP_H

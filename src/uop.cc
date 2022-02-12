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
 * File         : uop.cc
 * Author       : Joonho
 * Date         : 02/5/2022
 * SVN          : $Id: uop.cc,v 1.5 2021-10-08 21:01:41 kacear Exp $:
 * Description  : NDP uop
 *********************************************************************************************/

#include <iostream>
#include <cassert>

#include "uop.h"
#include "cxlsim.h"
#include "global_types.h"

namespace cxlsim {

uop_s::uop_s(cxlsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void uop_s::init(void) {
  m_core_id = -1;
  m_uop_type = UOP_NOP;
  m_mem_type = NOT_MEM;
  m_valid = false;
  m_addr = 0;
  m_latency = 0;
  m_last_dep_exec = 0;
  m_exec_cycle = 0;
  m_done_cycle = 0;
  m_src_rdy = false;
  m_src_cnt = 0;
  for (int ii = 0; ii < MAX_UOP_SRC_DEPS; ii++) {
    m_map_src_info[ii].m_uop = NULL;
  }
}

void uop_s::init_uop(int core_id, Counter unique_id, int uop_type, int mem_type, Addr addr,
    int latency) {
  m_core_id = core_id;
  m_unique_num = unique_id;
  m_uop_type = (Uop_Type)(uop_type);
  m_mem_type = (Mem_Type)(mem_type);
  m_valid = true;
  m_addr = addr;
  m_latency = latency;
  m_src_rdy = false;
}

bool uop_s::is_write(void) {
  return m_mem_type == MEM_ST;
}

Exec_Unit_Type uop_s::get_exec_unit() {
  switch (m_uop_type) {
    case UOP_INV:
    case UOP_SPEC:
    case UOP_NOP:
    case UOP_LFENCE:
    case UOP_FULL_FENCE:
    case UOP_ACQ_FENCE:
    case UOP_REL_FENCE:
    case UOP_X87:
    case UOP_XSAVE:
    case UOP_XSAVEOPT:
      return EXEC_NOP;
    case UOP_IADD:
    case UOP_VADD:
      return EXEC_IADD;
    case UOP_IMUL:
      return EXEC_IMUL;
    case UOP_IDIV:
      return EXEC_IDIV;
    case UOP_CF:
    case UOP_CMOV:
    case UOP_LDA:
    case UOP_ICMP:
    case UOP_LOGIC:
    case UOP_SHIFT:
    case UOP_BYTE:
    case UOP_MM:
    case UOP_VSTR:
    case UOP_SSE:
      return EXEC_IMISC;
    case UOP_FADD:
    case UOP_VFADD:
      return EXEC_FADD;
    case UOP_FMUL:
      return EXEC_FMUL;
    case UOP_FDIV:
      return EXEC_FDIV;
    case UOP_FCF:
    case UOP_FCVT:
    case UOP_FCMP:
    case UOP_FBIT:
    case UOP_FCMOV:
    case UOP_AES:
    case UOP_PCLMUL:
      return EXEC_FMISC;
    case UOP_IMEM:
    case UOP_FMEM:
    case UOP_LD:
    case UOP_ST:
      return EXEC_CACHE;
    default:
      assert(0);
      break;
  }
}

void uop_s::print(void) {
  std::cout << "UOP | id: " << m_unique_num
            << " val: " << m_valid
            << " srcrdy: " << m_src_rdy
            << " exe: " << m_exec_cycle
            << " done: " << m_done_cycle
            << " lat: " << (m_done_cycle - m_exec_cycle);

  for (int ii = 0; ii < MAX_UOP_SRC_DEPS; ii++) {
    auto src_uop = m_map_src_info[ii].m_uop;
    if (src_uop == NULL) {
      continue;
    }

    std::cout << " | id: " << src_uop->m_unique_num;
/* << " val: " << src_uop->m_valid */
/* << " exe: " << src_uop->m_exec_cycle */
/* << " done: " << src_uop->m_done_cycle; */
  }
  std::cout << std::endl;
}

} // namespace cxlsim

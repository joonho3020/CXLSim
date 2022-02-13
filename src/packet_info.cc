/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

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
 * File         : packet_info.cc
 * Author       : Joonho
 * Date         : 10/08/2021
 * SVN          : $Id: packet_info.h,v 1.5 2021-10-10 21:01:41 kacear Exp $:
 * Description  : PCIe packet information
 *********************************************************************************************/

#include <iostream>
#include <string>

#include "pcie_endpoint.h"
#include "packet_info.h"
#include "global_types.h"

namespace cxlsim {

//////////////////////////////////////////////////////////////////////////////

cxl_req_s::cxl_req_s(cxlsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void cxl_req_s::init(void) {
  m_id = 0;
  m_dram_start = 0;
  m_addr = 0;
  m_write = false;
  m_isuop = false;
  m_uop = NULL;
  m_req = NULL;
}

void cxl_req_s::init(Addr addr, bool write, bool isuop, uop_s* uop, void* req) {
  m_addr = addr;
  m_dram_start = 0;
  m_write = write;
  m_isuop = isuop;
  m_uop = uop;
  m_req = req;
}

void cxl_req_s::print(void) {
  return;
}

//////////////////////////////////////////////////////////////////////////////

message_s::message_s(cxlsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void message_s::init(void) {
  m_id = 0;
  m_bits = 0;
  m_type = INVALID;

  m_data = false;
  m_parent = NULL;
  m_childs.clear();
  m_arrived_child = 0;

  m_txvc_start = 0;
  m_txdll_start = 0;
  m_rxvc_start = 0;

  m_txtrans_end = 0;
  m_rxtrans_end = 0;

  m_vc_id = -1;
  m_req = NULL;
}

void message_s::print(void) {
  Addr addr = m_req ? m_req->m_addr : 0x00;
  std::string msg_type = m_data ? "DATA" 
                                : (m_vc_id == WD_CHANNEL) ? "WD"
                                : (m_vc_id == UOP_CHANNEL) ? "UOP"
                                : "WOD";

  std::cout << " " << std::hex << addr << ":" << msg_type;
}

//////////////////////////////////////////////////////////////////////////////

slot_s::slot_s(cxlsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void slot_s::init(void) {
  m_id = -1;
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] = 0;
  }
  m_type = INVAL_SLOT;
  m_msgs.clear();
}

int slot_s::size(void) {
  return (int)m_msgs.size();
}

int slot_s::get_wd_msg_cnt(void) {
  return m_msg_cnt[M2S_RWD] + m_msg_cnt[S2M_DRS];
}


void slot_s::print(void) {
  std::cout << "| " << m_id << ":";
  for (auto msg : m_msgs) {
    msg->print();
  }
}

//////////////////////////////////////////////////////////////////////////////

flit_s::flit_s(cxlsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void flit_s::init(void) {
  m_id = 0;
  m_bits = 0;
  m_phys_sent = false;
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] = 0;
  }

  m_txdll_end = 0;
  m_phys_end = 0;
  m_rxdll_end = 0;

  m_slots.clear();
}

int flit_s::size(void) {
  return (int)m_slots.size();
}

bool flit_s::is_rollover(void) {
  return (size() > 0) && (size() < *KNOB(KNOB_PCIE_DATA_SLOTS_PER_FLIT)) && 
    (m_slots.front()->m_type == G0);
}

void flit_s::push_back(slot_s* slot) {
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] += slot->m_msg_cnt[ii];
  }
  m_slots.push_back(slot);
}

void flit_s::push_front(slot_s* slot) {
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] += slot->m_msg_cnt[ii];
  }
  m_slots.push_front(slot);
}

void flit_s::print(void) {
  std::cout << "<FLIT> ";
  for (auto slot : m_slots) {
    slot->print();
  }
  std::cout << std::endl;
}

//////////////////////////////////////////////////////////////////////////////

} // namespace CXL

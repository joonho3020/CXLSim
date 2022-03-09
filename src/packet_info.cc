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
#include <cassert>

#include "pcie_endpoint.h"
#include "all_knobs.h"
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
  m_addr = 0;
  m_write = false;
  m_req = NULL;
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

  m_txvc_insert_start = 0;
  m_txvc_insert_done = 0;
  m_rxvc_insert_start = 0;
  m_rxvc_insert_done = 0;

  m_vc_id = -1;
  m_req = NULL;
}

bool message_s::is_wdata_msg(void) {
  return m_vc_id == WD_CHANNEL;
}

bool message_s::txvc_rdy(Counter cycle) {
  return m_txvc_insert_done <= cycle;
}
bool message_s::rxvc_rdy(Counter cycle) {
  return m_rxvc_insert_done <= cycle;
}

void message_s::init_data_msg(message_s* parent) {
  assert(parent->m_type == M2S_RWD || parent->m_type == S2M_DRS);

  m_bits = *KNOB(KNOB_PCIE_DATA_MSG_BITS);
  m_type = (parent->m_type == M2S_RWD) 
            ? M2S_DATA
            : S2M_DATA;
  m_data = true;
  m_parent = parent;
  m_parent->m_childs.push_back(this);
  m_vc_id = DATA_CHANNEL;
  m_req = NULL;
}

void message_s::inc_arrived_child(void) {
  m_arrived_child++;
}

bool message_s::child_waiting(void) {
  return (m_arrived_child < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT));
}

void message_s::print(void) {
  Addr addr = m_req ? m_req->m_addr : m_parent->m_req->m_addr;
  std::cout << "(" << addr << ":" << msg_type_string[m_type] << ") ";
}

//////////////////////////////////////////////////////////////////////////////

slot_s::slot_s(cxlsim_c* simBase) {
  m_simBase = simBase;
}

void slot_s::init(void) {
  m_id = 0;
  m_bits = 0;
  m_head = false;
  m_type = INVAL_SLOT;
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] = 0;
  }
  m_msgs.clear();
}

void slot_s::push_back(message_s* msg) {
  m_bits += msg->m_bits;
  m_msg_cnt[msg->m_type]++;
  m_msgs.push_back(msg);
}

bool slot_s::is_data(void) {
  return m_type == G0;
}

bool slot_s::empty(void) {
  return m_msgs.empty();
}

bool slot_s::multi_msg(void) {
  int cnt = 0;
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    if (m_msg_cnt[ii]) cnt++;
  }
  if (cnt > 1) return true;
  else return false;
}

void slot_s::assign_type(void) {
  if (m_head) {
    if (m_msg_cnt[M2S_REQ]) m_type = H5;
    else if (m_msg_cnt[M2S_RWD]) m_type = H4;
    else if (m_msg_cnt[S2M_DRS]) m_type = H5;
    else if (m_msg_cnt[S2M_NDR]) m_type = H4;
    else m_type = INVAL_SLOT;
  } else {
    if (m_msg_cnt[M2S_REQ]) m_type = G4;
    else if (m_msg_cnt[M2S_RWD]) m_type = G5;
    else if (m_msg_cnt[S2M_DRS] && m_msg_cnt[S2M_NDR]) m_type = G4;
    else if (m_msg_cnt[S2M_NDR]) m_type = G5;
    else if (m_msg_cnt[S2M_DRS]) m_type = G6;
    else if (m_msg_cnt[M2S_DATA] || m_msg_cnt[S2M_DATA]) m_type = G0;
    else m_type = INVAL_SLOT;
  }
}

void slot_s::set_head(void) {
  m_head = true;
}

#ifdef DEBUG
int slot_s::get_req_resp(void) {
  return m_msg_cnt[M2S_REQ] + m_msg_cnt[M2S_RWD] +
         m_msg_cnt[S2M_NDR] + m_msg_cnt[S2M_DRS];
}
#endif

void slot_s::print(void) {
  std::cout << "{" << slot_type_str[m_type] << " ";
  for (auto msg : m_msgs) {
    msg->print();
  }
  std::cout << "} ";
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

  m_flit_gen_cycle = 0;

  m_txreplay_insert_start = 0;
  m_txreplay_insert_done = 0;
  m_phys_start = 0;
  m_phys_done = 0;
  m_rxdll_done = 0;

#ifdef DEBUG
  m_reqresp_cnt = 0;
#endif

  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] = 0;
  }

  m_slots.clear();
}

int flit_s::num_slots(void) {
  return (int)m_slots.size();
}

void flit_s::push_back(slot_s* slot) {
  m_bits += slot->m_bits;
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] += slot->m_msg_cnt[ii];
  }
  m_slots.push_back(slot);

#ifdef DEBUG
  m_reqresp_cnt += slot->get_req_resp();
#endif
}

#ifdef DEBUG
int flit_s::get_req_resp() {
  int sum = 0;
  for (auto slot : m_slots) {
    sum += slot->m_msg_cnt[M2S_REQ] + slot->m_msg_cnt[M2S_RWD]
          + slot->m_msg_cnt[S2M_DRS] + slot->m_msg_cnt[S2M_NDR];
  }
  assert(sum == m_reqresp_cnt);

  return m_reqresp_cnt;
}
#endif

void flit_s::push_front(slot_s* slot) {
  m_bits += slot->m_bits;
  for (int ii = 0; ii < MAX_MSG_TYPES; ii++) {
    m_msg_cnt[ii] += slot->m_msg_cnt[ii];
  }
  m_slots.push_front(slot);

#ifdef DEBUG
  m_reqresp_cnt += slot->get_req_resp();
#endif
}

bool flit_s::rollover(void) {
  for (auto slot : m_slots) {
    // there is a slot that is not a data slot
    if (slot->m_type != G0) {
      return false;
    }
  }
  return (num_slots() < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT));
}

void flit_s::print(void) {
  std::cout << "FLT<";
  for (auto slot : m_slots) {
    slot->print();
  }
  std::cout << std::dec << m_bits << " " << m_phys_done << " " << m_rxdll_done << " " << m_flit_gen_cycle;
  std::cout << " >" << std::endl;
}

//////////////////////////////////////////////////////////////////////////////

} // namespace CXL

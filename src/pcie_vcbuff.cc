/*
Copyright (c) <2021>, <Seoul National University> All rights reserved.

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
 * File         : pcie_vcbuff.cc
 * Author       : Joonho
 * Date         : 2/15/2022
 * SVN          : $Id: pcie_vcbuff.cc 867 2021-10-08 02:28:12Z kacear $:
 * Description  : PCIe virtual channel buffer
 *********************************************************************************************/

#include <cassert>
#include <iostream>
#include <algorithm>

#include "pcie_vcbuff.h"

#include "utils.h"
#include "all_knobs.h"
#include "all_stats.h"
#include "assert_macros.h"

namespace cxlsim {

int vc_buff_c::m_msg_uid;
int vc_buff_c::m_slot_uid;
int vc_buff_c::m_flit_uid;

vc_buff_c::vc_buff_c(cxlsim_c* simBase) {
  m_simBase = simBase;
  m_cycle = 0;

  for (int ii = 0; ii < MAX_CHANNEL; ii++) {
    m_channel_cnt[ii] = 0;
  }
}

void vc_buff_c::init(bool is_tx, bool is_master, 
                        pool_c<message_s>* msg_pool, 
                        pool_c<slot_s>* slot_pool, 
                        pool_c<flit_s>* flit_pool,
                        int capacity) {
  m_istx = is_tx;
  m_master = is_master;
  m_msg_pool = msg_pool;
  m_slot_pool = slot_pool;
  m_flit_pool = flit_pool;
  m_channel_cap = capacity;

  m_hslot_msg_limit[M2S_REQ] = 1;
  m_hslot_msg_limit[M2S_RWD] = 1;
  m_hslot_msg_limit[S2M_DRS] = 2;
  m_hslot_msg_limit[S2M_NDR] = 2;

  m_gslot_msg_limit[M2S_REQ] = 1;
  m_gslot_msg_limit[M2S_RWD] = 1;
  m_gslot_msg_limit[S2M_DRS] = 2;
  m_gslot_msg_limit[S2M_NDR] = 2;

  m_flit_msg_limit[M2S_REQ] = 2;
  m_flit_msg_limit[M2S_RWD] = 1;
  m_flit_msg_limit[S2M_DRS] = 3;
  m_flit_msg_limit[S2M_NDR] = 2;
}

bool vc_buff_c::full(int vc_id) {
  return (m_channel_cnt[vc_id] >= m_channel_cap);
}

bool vc_buff_c::empty(int vc_id) {
  return (m_channel_cnt[vc_id] == 0);
}

int vc_buff_c::free(int vc_id) {
  return m_channel_cap - m_channel_cnt[vc_id];
}

int vc_buff_c::get_channel(cxl_req_s* req) {
  if (req->m_write) return m_master ? WD_CHANNEL : WOD_CHANNEL;
  else return m_master ? WOD_CHANNEL : WD_CHANNEL;
}

void vc_buff_c::insert(cxl_req_s* req) {
  int channel = get_channel(req);
  auto new_msg = acquire_message(channel, req);
  insert_channel(channel, new_msg);
}

flit_s* vc_buff_c::peek_flit() {
  if (m_flit_buff.empty()) {
    return NULL;
  }
  return m_flit_buff.front();
}

void vc_buff_c::pop_flit() {
  assert(!m_flit_buff.empty());
  m_flit_buff.pop_front();
}

message_s* vc_buff_c::pull_msg(int vc_id) {
  assert(!m_istx);
  for (auto msg : m_msg_buff) {
    if ((msg->m_vc_id != vc_id) || (msg->m_rxvc_insert_done > m_cycle) ||
        (msg->is_wdata_msg() && msg->child_waiting())) {
      continue;
    }
    remove_msg(msg);
    return msg;
  }
  return NULL;
}

void vc_buff_c::receive_flit(flit_s* flit) {
  for (auto slot : flit->m_slots) {
    for (auto msg : slot->m_msgs) {
      if (msg->m_data) {
        msg->m_parent->inc_arrived_child();
        release_msg(msg);
      } else {
        insert_channel(msg->m_vc_id, msg);
      }
    }
    release_slot(slot);
  }
  release_flit(flit);
}

void vc_buff_c::run_a_cycle() {
  m_cycle++;
}

void vc_buff_c::generate_flits() {
  std::list<message_s*> rdy;
  for (auto msg : m_msg_buff) {
    if ((m_istx && msg->txvc_rdy(m_cycle)) ||
        (!m_istx && msg->rxvc_rdy(m_cycle))) {
      rdy.push_back(msg);
    }
  }

  if ((int)rdy.size() == 0) {
    return;
  }

  // flit buffer is empty : generate new flit
  if (m_flit_buff.size() == 0) {
    generate_new_flit(rdy);
  } else {
    auto back_flit = m_flit_buff.back();

    // data rollover : insert a header slot
    if (back_flit->rollover()) {
      auto hslot = generate_hslot(rdy);
      if (hslot != NULL) {
        back_flit->push_front(hslot);
        add_data_slots_and_insert(back_flit, hslot);
      }
    } 
    // no rollover but not full : push general slot to back
    else if (back_flit->num_slots() < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT)) {
      auto gslot = generate_gslot(rdy, back_flit);
      if (gslot != NULL) {
        back_flit->push_back(gslot);
        add_data_slots_and_insert(back_flit, gslot);
      }
    } 
    // no rollover and full : generate a new flit
    else {
      generate_new_flit(rdy);
    }
  }
}

void vc_buff_c::generate_new_flit(std::list<message_s*>& msgs) {
  flit_s* new_flit = NULL;
  slot_s* hslot = generate_hslot(msgs);
  if (hslot != NULL) {
    new_flit = acquire_flit();
    new_flit->push_back(hslot);

    for (int ii = 0; ii < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT) - 1; ii++) {
      if ((int)msgs.size() == 0) {
        break;
      }
      slot_s* gslot = generate_gslot(msgs, new_flit);
      if (gslot != NULL) {
        new_flit->push_back(gslot);
      }
    }

    m_flit_buff.push_back(new_flit);

    // check if the current flit contains resp/req w/ data
    add_data_slots_and_insert(new_flit);
  }
}

void vc_buff_c::add_data_slots_and_insert(flit_s* flit, slot_s* slot) {
  std::list<slot_s*> data_slots;
  for (auto msg : slot->m_msgs) {
    if (!msg->is_wdata_msg()) {
      continue;
    }
    for (int ii = 0; ii < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT); ii++) {
      auto data_msg = acquire_message(DATA_CHANNEL, NULL);
      data_msg->init_data_msg(msg);

      auto data_slot = acquire_slot();
      data_slot->push_back(data_msg);
      data_slot->assign_type();
      data_slots.push_back(data_slot);
    }
  }

  insert_data_slots(flit, data_slots);
}

void vc_buff_c::add_data_slots_and_insert(flit_s* flit) {
  std::list<slot_s*> data_slots;
  for (auto slot : flit->m_slots) {
    for (auto msg : slot->m_msgs) {
      if (!msg->is_wdata_msg()) {
        continue;
      }
      for (int ii = 0; ii < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT); ii++) {
        auto data_msg = acquire_message(DATA_CHANNEL, NULL);
        data_msg->init_data_msg(msg);

        auto data_slot = acquire_slot();
        data_slot->push_back(data_msg);
        data_slot->assign_type();
        data_slots.push_back(data_slot);
      }
    }
  }

  insert_data_slots(flit, data_slots);
}

void vc_buff_c::insert_data_slots(flit_s* flit, std::list<slot_s*>& data_slots) {
  flit_s* new_flit = NULL;
  for (auto data_slot : data_slots) {
    if (flit->num_slots() < *KNOB(KNOB_PCIE_SLOTS_PER_FLIT)) {
      flit->push_back(data_slot);
    } else {
      if (new_flit == NULL) {
        new_flit = acquire_flit();
      }
      new_flit->push_back(data_slot);
      if (new_flit->num_slots() == *KNOB(KNOB_PCIE_SLOTS_PER_FLIT)) {
        m_flit_buff.push_back(new_flit);
        new_flit = NULL;
      }
    }
  }
  if (new_flit != NULL) {
    m_flit_buff.push_back(new_flit);
  }
}

slot_s* vc_buff_c::generate_hslot(std::list<message_s*>& msgs) {
  assert(!msgs.empty());

  // wait for a certain period before inserting into a flit
  auto msg = msgs.front();
  if ((m_cycle - msg->m_txvc_insert_done) < 
        *KNOB(KNOB_PCIE_MAX_FLIT_WAIT_CYCLE)) { 
    return NULL;
  }

  slot_s* new_slot = NULL;
  std::vector<message_s*> tmp;
  for (auto msg : msgs) {
    if (check_valid_header(new_slot, msg)) {
      if (new_slot == NULL) {
        new_slot = acquire_slot();
        new_slot->set_head();
      }
      new_slot->push_back(msg);
      tmp.push_back(msg);
    }
  }

  if (new_slot != NULL)
    new_slot->assign_type();

  for (auto msg : tmp) {
    msgs.remove(msg);
    remove_msg(msg);
  }
  return new_slot;
}

slot_s* vc_buff_c::generate_gslot(std::list<message_s*>& msgs, flit_s* flit) {
  slot_s* new_slot = NULL;
  std::vector<message_s*> tmp;
  for (auto msg : msgs) {
    if (check_valid_general(new_slot, msg, flit)) {
      if (new_slot == NULL) {
        new_slot = acquire_slot();
      }
      new_slot->push_back(msg);
      tmp.push_back(msg);
    }
  }

  if (new_slot != NULL)
    new_slot->assign_type();

  for (auto msg : tmp) {
    msgs.remove(msg);
    remove_msg(msg);
  }
  return new_slot;
}

bool vc_buff_c::check_valid_header(slot_s* slot, message_s* msg) {
  if (slot == NULL || slot->empty()) {
    return true;
  } else {
    if (slot->m_msg_cnt[msg->m_type] != 0) { // check limit
      return (slot->m_msg_cnt[msg->m_type] < m_hslot_msg_limit[msg->m_type]);
    } else { // headers only allow same type of message
      return false;
    }
  }
}

// FIXME : fix the hardcoded limits
bool vc_buff_c::check_valid_general(slot_s* slot, message_s* msg, flit_s* flit) {
  bool flit_chk = (flit->m_msg_cnt[msg->m_type] < m_flit_msg_limit[msg->m_type]);

  bool slot_chk = false;
  if (slot == NULL || slot->empty()) {
    slot_chk = true;
  } else {
    if (slot->multi_msg() || slot->m_msg_cnt[msg->m_type] == 0) {
      if (msg->m_type == S2M_NDR) {
        if (slot->m_msg_cnt[S2M_DRS] < 2 && slot->m_msg_cnt[S2M_NDR] < 2)
          slot_chk = true;
        else
          slot_chk = false;
      } else if (msg->m_type == S2M_DRS) {
        if (slot->m_msg_cnt[S2M_DRS] < 1 && slot->m_msg_cnt[S2M_NDR] < 3)
          slot_chk = true;
        else
          slot_chk = false;
      } else {
        slot_chk = false;
      }
    } else {
     slot_chk = (slot->m_msg_cnt[msg->m_type] < m_gslot_msg_limit[msg->m_type]);
    }
  }
  return slot_chk && flit_chk;
}

void vc_buff_c::insert_channel(int vc_id, message_s* msg) {
  m_channel_cnt[msg->m_vc_id]++;
  m_msg_buff.push_back(msg);
  if (m_istx) {
    msg->m_txvc_insert_start = m_cycle;
    msg->m_txvc_insert_done = m_cycle + *KNOB(KNOB_PCIE_TXTRANS_LATENCY);
  } else {
    msg->m_rxvc_insert_start = m_cycle;
    msg->m_rxvc_insert_done = m_cycle + *KNOB(KNOB_PCIE_RXTRANS_LATENCY);
  }
}

void vc_buff_c::remove_msg(message_s* msg) {
  assert(m_channel_cnt[msg->m_vc_id] > 0);
  m_channel_cnt[msg->m_vc_id]--;
  m_msg_buff.remove(msg);
}

void vc_buff_c::release_flit(flit_s* flit) {
  flit->init();
  m_flit_pool->release_entry(flit);
}

void vc_buff_c::release_slot(slot_s* slot) {
  slot->init();
  m_slot_pool->release_entry(slot);
}

void vc_buff_c::release_msg(message_s* msg) {
  msg->init();
  m_msg_pool->release_entry(msg);
}

message_s* vc_buff_c::acquire_message(int vc_id, cxl_req_s* req) {
  message_s* msg = m_msg_pool->acquire_entry(m_simBase);

  msg->init();
  msg->m_id = ++m_msg_uid;
  msg->m_vc_id = vc_id;
  msg->m_req = req;

  if (m_master) {
    switch (vc_id) {
      case WOD_CHANNEL:
        msg->m_type = M2S_REQ;
        msg->m_bits = *KNOB(KNOB_PCIE_REQ_MSG_BITS);
        break;
      case WD_CHANNEL:
        msg->m_type = M2S_RWD;
        msg->m_bits = *KNOB(KNOB_PCIE_RWD_MSG_BITS);
        break;
      case DATA_CHANNEL:
        msg->m_type = M2S_DATA;
        msg->m_bits = *KNOB(KNOB_PCIE_DATA_MSG_BITS);
        break;
      default:
        assert(0);
        break;
    }
  } else {
    switch (vc_id) {
      case WOD_CHANNEL:
        msg->m_type = S2M_NDR;
        msg->m_bits = *KNOB(KNOB_PCIE_NDR_MSG_BITS);
        break;
      case WD_CHANNEL:
        msg->m_type = S2M_DRS;
        msg->m_bits = *KNOB(KNOB_PCIE_DRS_MSG_BITS);
        break;
      case DATA_CHANNEL:
        msg->m_type = S2M_DATA;
        msg->m_bits = *KNOB(KNOB_PCIE_DATA_MSG_BITS);
        break;
      default:
        assert(0);
        break;
    }
  }
  return msg;
}

slot_s* vc_buff_c::acquire_slot() {
  slot_s* new_slot = m_slot_pool->acquire_entry(m_simBase);
  new_slot->init();
  new_slot->m_id = ++m_slot_uid;
  return new_slot;
}

flit_s* vc_buff_c::acquire_flit() {
  flit_s* new_flit = m_flit_pool->acquire_entry(m_simBase);
  new_flit->init();
  new_flit->m_id = ++m_flit_uid;
  return new_flit;
}

void vc_buff_c::print() {
  for (auto msg : m_msg_buff) {
    msg->print();
  }
  std::cout << std::endl;
  for (auto flit : m_flit_buff) {
    flit->print();
  }
}

} // namespace CXL

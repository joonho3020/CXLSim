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
 * File         : pcie_endpoint.cc
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: cxl_t3.h 867 2021-10-08 02:28:12Z kacear $:
 * Description  : PCIe endpoint device
 *********************************************************************************************/

#include <cassert>
#include <iostream>
#include <algorithm>

#include "pcie_endpoint.h"

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
  if (req->m_isuop) return UOP_CHANNEL;
  else if (req->m_write) return m_master ? WD_CHANNEL : WOD_CHANNEL;
  else return m_master ? WOD_CHANNEL : WD_CHANNEL;
}

void vc_buff_c::insert(cxl_req_s* req) {
  int channel = get_channel(req);
  auto new_msg = m_msg_pool->acquire_entry(m_simBase);
  init_new_msg(new_msg, channel, req);
  insert_channel(channel, new_msg);
}

// FIXME
void vc_buff_c::generate_flits() {
  std::vector<message_s*> rdy;
  for (auto msg : m_msg_buff) {
    if ((m_istx && msg->txvc_rdy(m_cycle)) ||
        (!m_istx && msg->rxvc_rdy(m_cycle))) {
      slot_s* new_slot = m_slot_pool->acquire_entry(m_simBase);
      new_slot->init();
      new_slot->m_id = ++m_slot_uid;
      new_slot->push_back(msg);

      rdy.push_back(msg);
      m_slot_buff.push_back(new_slot);
    }
  }

  for (auto msg : rdy) {
    remove_msg(msg);
  }

  flit_s* new_flit = NULL;
  std::vector<slot_s*> tmp;
  for (auto slot : m_slot_buff) {
    if (new_flit == NULL) {
      new_flit = m_flit_pool->acquire_entry(m_simBase);
      new_flit->init();
      new_flit->m_id = ++m_flit_uid;
    }
    new_flit->push_back(slot);
    tmp.push_back(slot);

    if (new_flit->num_slots() == *KNOB(KNOB_PCIE_SLOTS_PER_FLIT)) {
      break;
    }
  }

  if (new_flit != NULL) {
    m_flit_buff.push_back(new_flit);
  }

  for (auto slot : tmp) {
    remove_slot(slot);
  }
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
    if ((msg->m_vc_id != vc_id) || (msg->m_rxvc_start > m_cycle)) {
      continue;
    }
    remove_msg(msg);
    return msg;
  }
  return NULL;
}

void vc_buff_c::init_new_msg(message_s* msg, int vc_id, cxl_req_s* req) {
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
      case UOP_CHANNEL:
        msg->m_type = M2S_UOP;
        msg->m_bits = *KNOB(KNOB_PCIE_UOP_MSG_BITS);
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
      case UOP_CHANNEL:
        msg->m_type = S2M_UOP;
        msg->m_bits = *KNOB(KNOB_PCIE_UOP_MSG_BITS);
        break;
      default:
        assert(0);
        break;
    }
  }
}

void vc_buff_c::insert_channel(int vc_id, message_s* msg) {
  m_channel_cnt[msg->m_vc_id]++;
  m_msg_buff.push_back(msg);
  if (m_istx) {
    msg->m_txvc_start = m_cycle + *KNOB(KNOB_PCIE_TXTRANS_LATENCY);
  } else {
    msg->m_rxvc_start = m_cycle + *KNOB(KNOB_PCIE_RXTRANS_LATENCY);
  }
}

void vc_buff_c::remove_msg(message_s* msg) {
  assert(m_channel_cnt[msg->m_vc_id] > 0);
  m_channel_cnt[msg->m_vc_id]--;
  m_msg_buff.remove(msg);
}

void vc_buff_c::remove_slot(slot_s* slot) {
  m_slot_buff.remove(slot);
}

void vc_buff_c::run_a_cycle() {
  m_cycle++;
}

void vc_buff_c::receive_flit(flit_s* flit) {
  for (auto slot : flit->m_slots) {
    for (auto msg : slot->m_msgs) {
      insert_channel(msg->m_vc_id, msg);
    }
    release_slot(slot);
  }
  release_flit(flit);
}

void vc_buff_c::release_flit(flit_s* flit) {
  flit->init();
  m_flit_pool->release_entry(flit);
}

void vc_buff_c::release_slot(slot_s* slot) {
  slot->init();
  m_slot_pool->release_entry(slot);
}

void vc_buff_c::print() {
  for (auto msg : m_msg_buff) {
    msg->print();
  }
  std::cout << std::endl;
  for (auto slot : m_slot_buff) {
    slot->print();
    std::cout << std::endl;
  }
  for (auto flit : m_flit_buff) {
    flit->print();
  }
}

//////////////////////////////////////////////////////////////////////////////

pcie_ep_c::pcie_ep_c(cxlsim_c* simBase) {
  // simulation related
  m_simBase = simBase;
  m_cycle = 0;

  // set memory request size
  m_lanes = *KNOB(KNOB_PCIE_LANES);
  m_perlane_bw = *KNOB(KNOB_PCIE_PER_LANE_BW);
  m_prev_txphys_cycle = 0;
  m_peer_ep = NULL;

  ASSERTM((m_lanes & (m_lanes - 1)) == 0, "number of lanes should be power of 2\n");

  m_txvc = new vc_buff_c(simBase);
  m_rxvc = new vc_buff_c(simBase);
  m_rxvc_bw = *KNOB(KNOB_PCIE_RXVC_BW);

/* m_flit_ndr_cnt = 0; */
/* m_flit_drs_cnt = 0; */
/* m_slot_ndr_cnt = 0; */
/* m_slot_drs_cnt = 0; */

/* m_slot_cnt = 0; */

/* m_max_flit_wait = *KNOB(KNOB_PCIE_MAX_FLIT_WAIT_CYCLE); */
/* m_flit_wait_cycle = 0; */

/* m_cur_flit = NULL; */

  // initialize dll
/* m_txdll_cap = *KNOB(KNOB_PCIE_TXDLL_CAPACITY); */
  m_txreplay_cap = *KNOB(KNOB_PCIE_TXREPLAY_CAPACITY);

  // initialize physical layers 
  // the number for consecutive flits that can be sent together 
  // varies by the number of lanes (see CXL spec 2.0 physical layer)
  switch (m_lanes) {
    case 16: 
      m_phys_cap = 4; 
      break;
    case 8: 
      m_phys_cap = 2; 
      break;
    case 4:
    case 2:
    case 1:
      m_phys_cap = 1;
      break;
    default:
      assert(0); // should be power of 2s
      break;
  }
}

pcie_ep_c::~pcie_ep_c() {
  delete m_txvc;
  delete m_rxvc;
}

void pcie_ep_c::init(int id, bool master, pool_c<message_s>* msg_pool, 
                     pool_c<slot_s>* slot_pool,
                     pool_c<flit_s>* flit_pool, pcie_ep_c* peer) {
  m_id = id;
  m_master = master;
  m_msg_pool = msg_pool;
  m_slot_pool = slot_pool;
  m_flit_pool = flit_pool;
  m_peer_ep = peer;

  int tx_cap = *KNOB(KNOB_PCIE_TXVC_CAPACITY);
  int rx_cap = *KNOB(KNOB_PCIE_RXVC_CAPACITY);
  m_txvc->init(/* tx? */true,  m_master, 
                m_msg_pool, m_slot_pool, m_flit_pool, tx_cap);
  m_rxvc->init(/* tx? */false, m_master, 
                m_msg_pool, m_slot_pool, m_flit_pool, tx_cap);
}

void pcie_ep_c::run_a_cycle(bool pll_locked) {
  // receive requests
  end_transaction();
  process_rxtrans();
  process_rxdll();
  process_rxphys();

  // send requests
  process_txphys();
  process_txdll();
  process_txtrans();
  start_transaction();

  m_cycle++;
}

bool pcie_ep_c::phys_layer_full() {
  return (m_phys_cap == (int)m_rxphys_q.size());
}

void pcie_ep_c::insert_phys(flit_s* flit) {
  ASSERTM(!phys_layer_full(), "physical layer should not be full");
  m_rxphys_q.push_back(flit);
}

bool pcie_ep_c::has_free_rxvc(int vc_id) {
  return (m_rxvc->full(vc_id) == false);
}

//////////////////////////////////////////////////////////////////////////////
// private

Counter pcie_ep_c::get_phys_latency(flit_s* flit) {
  float freq = *KNOB(KNOB_CLOCK_IO);
  int lanes = *KNOB(KNOB_PCIE_LANES);
  return static_cast<Counter>(flit->m_bits / (lanes * m_perlane_bw) * freq);
}

/* bool pcie_ep_c::dll_layer_full(bool tx) { */
/* if (tx) { */
/* return (m_txdll_cap <= (int)m_txdll_q.size()); */
/* } else { */
/* assert(0); */
/* } */
/* } */

/* void pcie_ep_c::init_new_flit(flit_s* flit, int bits) { */
/* flit->init(); */
/* flit->m_id = ++m_flit_uid; */
/* flit->m_bits = bits; */
/* } */

// FIXME
void pcie_ep_c::parse_and_insert_flit(flit_s* flit) {
/* Counter rxdll_start = flit->m_phys_end; */

/* for (auto msg : flit->m_msgs) { */
/* if (msg->m_data) { */
/* ASSERTM(msg->m_parent, "Data message should have a parent req"); */

/* msg->m_parent->m_arrived_child++; */
/* } else { */
/* int vc_id = msg->m_vc_id; */
/* ASSERTM(m_rxvc_cap > (int)m_rxvc_buff[vc_id].size(), */
/* "Due to flow control, rxvc should always have free space"); */

/* msg->m_rxtrans_end = m_cycle + *KNOB(KNOB_PCIE_RXTRANS_LATENCY); */
/* m_rxvc_buff[vc_id].push_back(msg); */
/* } */

/* // for stats */
/* msg->m_rxvc_start = m_cycle; */

/* // update stats */
/* STAT_EVENT(PCIE_RXDLL_BASE); */
/* STAT_EVENT_N(AVG_PCIE_RXDLL_LATENCY, m_cycle - rxdll_start); */
/* } */

/* release_flit(flit); */
}

void pcie_ep_c::refresh_replay_buffer() {
  while (m_txreplay_buff.size()) {
    flit_s* flit = m_txreplay_buff.front();

    // if the flit is send && the flit is received by the peer
    if (flit->m_phys_sent && flit->m_phys_end <= m_cycle) {
      m_txreplay_buff.pop_front();
    } else {
      break;
    }
  }
}

/* void pcie_ep_c::add_and_push_data_msg(message_s* msg) { */
/* assert(msg->m_req); */
/* int data_slots = *KNOB(KNOB_PCIE_DATA_SLOTS_PER_FLIT); */

/* for (int ii = 0; ii < data_slots; ii++) { */
/* message_s* new_data_msg = m_msg_pool->acquire_entry(m_simBase); */

/* init_new_msg(new_data_msg, DATA_CHANNEL, NULL); */

/* new_data_msg->m_data = true; */
/* new_data_msg->m_parent = msg; */
/* new_data_msg->m_txtrans_end = m_cycle + *KNOB(KNOB_PCIE_TXTRANS_LATENCY); */
/* new_data_msg->m_txdll_start = m_cycle; */

/* msg->m_childs.push_back(new_data_msg); */
/* m_txdll_q.push_back(new_data_msg); */

/* assert(msg != new_data_msg); */
/* assert(new_data_msg->m_childs.size() == 0); */
/* } */
/* } */

void pcie_ep_c::release_msg(message_s* msg) {
  msg->init();
  m_msg_pool->release_entry(msg);
}

//////////////////////////////////////////////////////////////////////////////
// protected

// virtual class
// should call push_txvc internally
// look at pcie_rc_c, cxl_t3_c for examples
void pcie_ep_c::start_transaction() {
  return;
}

// used for start_transaction
bool pcie_ep_c::push_txvc(cxl_req_s* cxl_req) {
  assert(cxl_req);
  int channel = m_txvc->get_channel(cxl_req);;

  if (m_txvc->full(channel)) {
    return false;
  } else {
    m_txvc->insert(cxl_req);
    return true;
  }
}

// virtual class
// should call pull_rxvc internally
// look at pcie_rc_c, cxl_t3_c for examples
void pcie_ep_c::end_transaction() {
  return;
}

// used for end_transaction
cxl_req_s* pcie_ep_c::pull_rxvc() {
  // choose the vc buffer with minimum free space left
  std::vector<std::pair<int, int>> candidate;
  for (int ii = 0; ii < MAX_CHANNEL; ii++) {
    if (m_rxvc->empty(ii)) {
      continue;
    } else {
      int remain = m_rxvc->free(ii);
      candidate.push_back({remain, ii});
    }
  }

  if ((int)candidate.size() == 0) {
    return NULL;
  }

  sort(candidate.begin(), candidate.end());
  for (auto cand : candidate) { // pull from the vc with least remaining entries
    int vc_id = cand.second;

    message_s* msg = m_rxvc->pull_msg(vc_id);
    if (msg == NULL) {
      continue;
    }
    cxl_req_s* req = msg->m_req;
    release_msg(msg);
    return req;
  }
  return NULL;
}

bool pcie_ep_c::check_peer_credit(message_s* pkt) {
  int vc_id = pkt->m_vc_id;
  return m_peer_ep->has_free_rxvc(vc_id);
}

bool pcie_ep_c::check_peer_credit(flit_s* flit) {
  bool success = true;
  for (auto slot : flit->m_slots) {
    for (auto msg : slot->m_msgs) {
      success = check_peer_credit(msg);
    }
  }
  return success;
}


//////////////////////////////////////////////////////////////////////////////

void pcie_ep_c::process_txtrans() {
  m_txvc->generate_flits();
  m_txvc->run_a_cycle();
}

// TODO : Replay buff bw
void pcie_ep_c::process_txdll() {
  while (m_txreplay_cap > (int)m_txreplay_buff.size()) {
    flit_s* flit = m_txvc->peek_flit();

    if (flit != NULL) {
      bool fctrl_success = check_peer_credit(flit);
      if (fctrl_success) {
        flit->m_txdll_end = m_cycle + *KNOB(KNOB_PCIE_TXDLL_LATENCY);
        m_txreplay_buff.push_back(flit);
        m_txvc->pop_flit();
      }
    } else {
      break;
    }
  }
}

void pcie_ep_c::process_txphys() {
  refresh_replay_buffer();

  if (!m_peer_ep->phys_layer_full()) {
    for (auto cur_flit : m_txreplay_buff) {
      if (cur_flit->m_phys_sent) { // already sent
        continue;
      } else if (cur_flit->m_txdll_end <= m_cycle) {
        // - packets are sent serially so transmission starts only after
        //   the previous packet finished physical layer transmission
        Counter lat = get_phys_latency(cur_flit) 
                      + 2*(*KNOB(KNOB_PCIE_ARBMUX_LATENCY)); // tx & rx
        Counter start_cyc = std::max(m_prev_txphys_cycle, m_cycle);
        Counter phys_finished = start_cyc + lat;

        m_prev_txphys_cycle = phys_finished;
        cur_flit->m_phys_end = phys_finished;
        cur_flit->m_rxdll_end = phys_finished + *KNOB(KNOB_PCIE_RXDLL_LATENCY);
        cur_flit->m_phys_sent = true;

        // push to peer endpoint physical
        m_peer_ep->insert_phys(cur_flit);

        // update dll stats
/* for (auto msg : cur_flit->m_msgs) { */
/* STAT_EVENT(PCIE_TXDLL_BASE); */
/* STAT_EVENT_N(AVG_PCIE_TXDLL_LATENCY, m_cycle - msg->m_txdll_start); */
/* } */

        // update goodput related stats
/* int good_bits = 0; */
/* for (auto msg : cur_flit->m_msgs) { */
/* good_bits += msg->m_bits; */
/* } */
/* STAT_EVENT_N(PCIE_GOODPUT_BASE, *KNOB(KNOB_PCIE_FLIT_BITS)); */
/* STAT_EVENT_N(AVG_PCIE_GOODPUT, good_bits); */

/* // update phys latency related stats */
/* STAT_EVENT(PCIE_FLIT_BASE); */
/* STAT_EVENT_N(AVG_PCIE_PHYS_LATENCY, phys_finished - cur_flit->m_txdll_end); */

        break;
      }
    }
  }
}

void pcie_ep_c::process_rxphys() {
  while (m_rxphys_q.size()) {
    flit_s* flit = m_rxphys_q.front();

    // finished physical layer & rx dll layer
    if (flit->m_rxdll_end <= m_cycle) {
      m_rxphys_q.pop_front();
      m_rxvc->receive_flit(flit);
    } else {
      break;
    }
  }
}

void pcie_ep_c::process_rxdll() {
  return;
}

void pcie_ep_c::process_rxtrans() {
  m_rxvc->run_a_cycle();
}

int pcie_ep_c::get_rxvc_bw() {
  return m_rxvc_bw;
}

void pcie_ep_c::print_ep_info() {
  std::cout << "======== TXVC" << std::endl;
  m_txvc->print();
  std::cout << std::endl;

  std::cout << "======= Replay buff" << std::endl;
  for (auto flit : m_txreplay_buff) {
    flit->print();
  }

  std::cout << "======= RX Physical" << std::endl;
  for (auto flit : m_rxphys_q) {
    flit->print();
  }

  std::cout << "======== RXVC" << std::endl;
  m_rxvc->print();
  std::cout << std::endl;
}

} // namespace CXL

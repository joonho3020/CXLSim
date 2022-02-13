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

#include "knob.h"
#include "pcie_endpoint.h"

#include "uop.h"
#include "utils.h"
#include "all_knobs.h"
#include "all_stats.h"
#include "assert_macros.h"

namespace cxlsim {

/////////////////////////////////////////////////////////////////////////////
// Public

int vc_buff_c::m_msg_uid;
int vc_buff_c::m_flit_uid;

vc_buff_c::vc_buff_c(cxlsim_c* simBase) {
  m_cycle = 0;
  m_simBase = simBase;
}

void vc_buff_c::init(bool tx, bool master, 
                      pool_c<message_s>* msg_pool,
                      pool_c<slot_s>* slot_pool,
                      pool_c<flit_s>* flit_pool) {
  for (int ii = 0; ii < MAX_CHANNEL; ii++) {
    m_ch_cnt[ii] = 0;
  }

  m_master = master;
  m_tx = tx;

  if (m_tx) m_ch_cap = *KNOB(KNOB_PCIE_TXVC_CAPACITY);
  else m_ch_cap = *KNOB(KNOB_PCIE_RXVC_CAPACITY);

  m_msg_pool = msg_pool;
  m_slot_pool = slot_pool;
  m_flit_pool = flit_pool;

  m_hslot_msg_limit[M2S_REQ] = 1;
  m_hslot_msg_limit[M2S_RWD] = 1;
  m_hslot_msg_limit[S2M_DRS] = 2;
  m_hslot_msg_limit[S2M_NDR] = 2;

  m_hslot_msg_limit[M2S_REQ] = 1;
  m_hslot_msg_limit[M2S_RWD] = 1;
  m_hslot_msg_limit[S2M_DRS] = 1;
  m_hslot_msg_limit[S2M_NDR] = 2;

  m_flit_msg_limit[M2S_REQ] = 2;
  m_flit_msg_limit[M2S_RWD] = 1;
  m_flit_msg_limit[S2M_DRS] = 3;
  m_flit_msg_limit[S2M_NDR] = 2;
}

bool vc_buff_c::insert_req(cxl_req_s* req) {
  int ch_id = get_channel(req);

  // channel full
  if (m_ch_cnt[ch_id] == m_ch_cap) {
    return false;
  }

  // insert into channel
  message_s* new_msg;
  new_msg = m_msg_pool->acquire_entry(m_simBase);
  init_new_msg(new_msg, ch_id, req);
  insert_msg(new_msg, ch_id);

  // TODO : stats

  return true;
}

std::vector<flit_s*>& vc_buff_c::pull_flit() {
  
}

void vc_buff_c::run_a_cycle() {
  if (m_tx) generate_slots();
  m_cycle++;
}

/////////////////////////////////////////////////////////////////////////////
// private functions

void vc_buff_c::insert_msg(message_s* msg, int ch_id) {
  m_ch_cnt[ch_id]++;
  m_msgs.push_back(msg);
  msg->m_txvc_start = m_cycle + *KNOB(KNOB_PCIE_VC_BUFF_LATENCY);
}

void vc_buff_c::rm_msg(message_s* msg) {
  int ch_id = msg->m_vc_id;
  assert(m_ch_cnt[ch_id] > 0);
  m_ch_cnt[ch_id]--;
  m_msgs.remove(msg);
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

int vc_buff_c::get_channel(cxl_req_s* req) {
  if (req->m_isuop) return UOP_CHANNEL;
  else if (req->m_write) return m_master ? WD_CHANNEL : WOD_CHANNEL;
  else return m_master ? WOD_CHANNEL : WD_CHANNEL;
}

void vc_buff_c::init_new_flit(flit_s* flit, int bits) {
  flit->init();
  flit->m_id = ++m_flit_uid;
  flit->m_bits = bits;
}

bool vc_buffer_c::check_header_slot_limit(slot_s* slt, message_s* msg) {
  return slt->m_msg_cnt[msg->m_type] < m_hslot_msg_limit[msg->m_type];
}

slot_s* vc_buff_c::generate_header_slot(std::vector<message_s*>& rdy_msgs) {
  assert((int)rdy_msgs.size() != 0);

  auto new_slot = m_slot_pool->acquire_entry(m_simBase);
  for (auto msg : rdy_msgs) {
    if (check_header_slot_limit(new_slot, msg) == false) {
      break;
    }
    new_slot->m_msg_cnt[msg->m_type]++;
    new_slot->m_msgs.push_back(msg);
    rm_msg(msg);
  }
  if (new_slot->size() == 0) {
    m_slot_pool->release_entry(new_slot);
    new_slot = NULL;
  }
  return new_slot;
}

bool vc_buff_c::check_general_slot_limit(flit_s* flit, slot_s* slt, 
                                        message_s* msg) {
  return (slt->m_msg_cnt[msg->m_type] < m_gslot_msg_limit[msg->m_type]) &&
    (flit->m_msg_cnt[msg->m_type] < m_flit_msg_limit[msg->m_type]);
}

slot_s* vc_buff_c::generate_general_slot(flit_s* flit, 
                                         std::vector<message_s*>& rdy_msgs) {
  assert((int)rdy_msgs.size() != 0);
  auto new_slot = m_slot_pool->acquire_entry(m_simBase);
  for (auto msg : rdy_msgs) {
    if (check_general_slot_limit(flit, new_slot, msg) == false) {
      break;
    }
    new_slot->m_msg_cnt[msg->m_type]++;
    new_slot->m_msgs.push_back(msg);
    rm_msg(msg);
  }
  if (new_slot->size() == 0) {
    m_slot_pool->release_entry(new_slot);
    new_slot = NULL;
  }
  return new_slot;
}


void vc_buff_c::generate_flits() {

  // get ready messages
  std::vector<message_s*> ready_msgs;
  for (auto msg : m_msgs) {
    if (msg->m_txvc_start <= m_cycle) { // message is ready
      ready_msgs.push_back(msg);
    }
  }

  if ((int)ready_msgs.size() == 0) return;

  // sort ready messages (?)

  bool generate_header = false;
  if (m_cur_flit == NULL) {
    flit_s* new_flit = m_flit_pool->acquire_entry(m_simBase);
    init_new_flit(new_flit, *KNOB(KNOB_PCIE_FLIT_BITS));
    m_cur_flit = new_flit;

    generate_header = true;
  } else {
    if (m_cur_flit->is_rollover()) {
      generate_header = true;
    }
  }

  slot_s* new_slot = NULL;
  if (generate_header) {
    new_slot = generate_header_slot(ready_msgs);
    assert(new_slot != NULL);
    m_cur_flit->push_front(new_slot);
  } else {
    new_slot = generate_general_slot(m_cur_flit, ready_msgs);
    if (new_slot != NULL) {
      m_cur_flit->push_back(new_slot);
    }
  }

  if ((new_slot != NULL) && 
      (new_slot->get_wd_msg_cnt() > 0 )) {
    for (auto msg : new_slot->m_msgs) {
    }
  }

}

/////////////////////////////////////////////////////////////////////////////

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
  m_txvc_rr_idx = 0;

  // initialize VC buffers & credit
  m_vc_cnt = *KNOB(KNOB_PCIE_VC_CNT);

  m_txvc_cap = *KNOB(KNOB_PCIE_TXVC_CAPACITY);
  m_rxvc_cap = *KNOB(KNOB_PCIE_RXVC_CAPACITY);
  m_txvc_buff = new std::list<message_s*>[m_vc_cnt];
  m_rxvc_buff = new std::list<message_s*>[m_vc_cnt];
  m_rxvc_bw = *KNOB(KNOB_PCIE_RXVC_BW);

  ASSERTM(m_vc_cnt == MAX_CHANNEL, "currently only 4 virtual channels exist\n");

  m_flit_ndr_cnt = 0;
  m_flit_drs_cnt = 0;
  m_slot_ndr_cnt = 0;
  m_slot_drs_cnt = 0;

  m_slot_cnt = 0;

  m_max_flit_wait = *KNOB(KNOB_PCIE_MAX_FLIT_WAIT_CYCLE);
  m_flit_wait_cycle = 0;

  m_cur_flit = NULL;

  // initialize dll
  m_txdll_cap = *KNOB(KNOB_PCIE_TXDLL_CAPACITY);
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
  delete[] m_txvc_buff;
  delete[] m_rxvc_buff;
}

void pcie_ep_c::init(int id, bool master, pool_c<message_s>* msg_pool, 
                     pool_c<flit_s>* flit_pool, pcie_ep_c* peer) {
  m_id = id;
  m_master = master;
  m_msg_pool = msg_pool;
  m_flit_pool = flit_pool;
  m_peer_ep = peer;
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

// We assume that the flow control CF flits has litle impact to performance
bool pcie_ep_c::check_peer_credit(message_s* pkt) {
  int vc_id = pkt->m_vc_id;
  return ((int)m_peer_ep->m_rxvc_buff[vc_id].size() < m_peer_ep->m_rxvc_cap);
}

//////////////////////////////////////////////////////////////////////////////
// private

Counter pcie_ep_c::get_phys_latency(flit_s* flit) {
  float freq = *KNOB(KNOB_CLOCK_IO);
  int lanes = *KNOB(KNOB_PCIE_LANES);
  return static_cast<Counter>(flit->m_bits / (lanes * m_perlane_bw) * freq);
}

bool pcie_ep_c::txdll_layer_full() {
  return (m_txdll_cap <= (int)m_txdll_q.size());
}


void pcie_ep_c::init_new_flit(flit_s* flit, int bits) {
  flit->init();
  flit->m_id = ++m_flit_uid;
  flit->m_bits = bits;
}

bool pcie_ep_c::txvc_not_full(int channel) {
  return (m_txvc_cap - (int)m_txvc_buff[channel].size() > 0);
}

void pcie_ep_c::parse_and_insert_flit(flit_s* flit) {
  Counter rxdll_start = flit->m_phys_end;

  for (auto msg : flit->m_msgs) {
    if (msg->m_data) {
      ASSERTM(msg->m_parent, "Data message should have a parent req");

      msg->m_parent->m_arrived_child++;
    } else {
      int vc_id = msg->m_vc_id;
      ASSERTM(m_rxvc_cap > (int)m_rxvc_buff[vc_id].size(),
              "Due to flow control, rxvc should always have free space");

      msg->m_rxtrans_end = m_cycle + *KNOB(KNOB_PCIE_RXTRANS_LATENCY);
      m_rxvc_buff[vc_id].push_back(msg);
    }

    // for stats
    msg->m_rxvc_start = m_cycle;

    // update stats
    STAT_EVENT(PCIE_RXDLL_BASE);
    STAT_EVENT_N(AVG_PCIE_RXDLL_LATENCY, m_cycle - rxdll_start);
  }

  release_flit(flit);
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

bool pcie_ep_c::is_wdata_msg(message_s* msg) {
  return (msg->m_vc_id == WD_CHANNEL);
}

// The reason why we do not implement a separate data channel is because
// it is only rational that a req/resp with data can be sent only when the
// corresponding data is ready.
// Hence, it is only reasonable to design HW such that all data channels have
// enough space when a req/resp with data is pushed into the trans layer.
void pcie_ep_c::add_and_push_data_msg(message_s* msg) {
  assert(msg->m_req);
  int data_slots = *KNOB(KNOB_PCIE_DATA_SLOTS_PER_FLIT);

  for (int ii = 0; ii < data_slots; ii++) {
    message_s* new_data_msg = m_msg_pool->acquire_entry(m_simBase);

    init_new_msg(new_data_msg, DATA_CHANNEL, NULL);

    new_data_msg->m_data = true;
    new_data_msg->m_parent = msg;
    new_data_msg->m_txdll_start = m_cycle + *KNOB(KNOB_PCIE_TXTRANS_LATENCY);

    msg->m_childs.push_back(new_data_msg);
    m_txdll_q.push_back(new_data_msg);

    assert(msg != new_data_msg);
    assert(new_data_msg->m_childs.size() == 0);
  }
}

int pcie_ep_c::get_channel(cxl_req_s* req) {
  if (req->m_isuop) return UOP_CHANNEL;
  else if (req->m_write) return m_master ? WD_CHANNEL : WOD_CHANNEL;
  else return m_master ? WOD_CHANNEL : WD_CHANNEL;
}

void pcie_ep_c::release_flit(flit_s* flit) {
  flit->init();
  m_flit_pool->release_entry(flit);
}

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
  int channel = get_channel(cxl_req);;

  message_s* new_msg;
  if (!txvc_not_full(channel)) { // txvc full
    return false;
  } else {
    new_msg = m_msg_pool->acquire_entry(m_simBase);
    init_new_msg(new_msg, channel, cxl_req);
    m_txvc_buff[channel].push_back(new_msg);

    // for stats
    new_msg->m_txvc_start = m_cycle + *KNOB(KNOB_PCIE_TXTRANS_LATENCY);;
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
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    // empty
    if ((int)m_rxvc_buff[ii].size() == 0) {
      continue;
    }
    // not empty 
    else {
      int remain = m_rxvc_cap - (int)m_rxvc_buff[ii].size();
      candidate.push_back({remain, ii});
    }
  }

  sort(candidate.begin(), candidate.end());

  // pull from the vc with least remaining entries
  for (auto cand : candidate) {
    int vc_id = cand.second;

    message_s* msg = m_rxvc_buff[vc_id].front();

    if (msg->m_rxtrans_end > m_cycle) {
      continue;
    } else {
      // if the message is not ready, continue
      if (is_wdata_msg(msg) && 
          msg->m_arrived_child != *KNOB(KNOB_PCIE_MAX_MSG_PER_FLIT)) {
        continue;
      }

      m_rxvc_buff[vc_id].pop_front();
      assert(msg->m_vc_id == vc_id);

      cxl_req_s* mem_req = msg->m_req;

      if (is_wdata_msg(msg)) {
        assert((int)msg->m_childs.size() != 0);

        // free the associated data messages
        for (auto child : msg->m_childs) {
          // update stats
          STAT_EVENT(PCIE_RXTRANS_BASE);
          STAT_EVENT_N(AVG_PCIE_RXTRANS_LATENCY, m_cycle - child->m_rxvc_start);

          release_msg(child);
        }
      }

      // update stats
      STAT_EVENT(PCIE_RXTRANS_BASE);
      STAT_EVENT_N(AVG_PCIE_RXTRANS_LATENCY, m_cycle - msg->m_rxvc_start);

      release_msg(msg);

      return mem_req;
    }
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void pcie_ep_c::process_txtrans() {
  // round robin policy
  for (int ii = m_txvc_rr_idx, cnt = 0; cnt < m_vc_cnt; 
      cnt++, ii = (ii + 1)  % m_vc_cnt) {

    if (m_txvc_buff[ii].empty()) { // VC buffer empty
      continue;
    } else { // VC buffer not empty
      message_s* msg = m_txvc_buff[ii].front();

      // still inserting into the VC
      if (msg->m_txvc_start > m_cycle) {
        break;
      }

      int vc_id = msg->m_vc_id;
      if (!check_peer_credit(msg)) { // check simple flow ctrl
        continue;
      } else {
        if (!txdll_layer_full()) {
          m_txvc_buff[vc_id].pop_front();
          m_txdll_q.push_back(msg);
          msg->m_txdll_start = m_cycle;

          // update tx trans latency related stats
          STAT_EVENT(PCIE_TXTRANS_BASE);
          STAT_EVENT_N(AVG_PCIE_TXTRANS_LATENCY, m_cycle - msg->m_txvc_start);

          // update associated data message stats
          if (is_wdata_msg(msg)) {
            int data_slots = *KNOB(KNOB_PCIE_DATA_SLOTS_PER_FLIT);
            STAT_EVENT_N(PCIE_TXTRANS_BASE, data_slots);
            STAT_EVENT_N(AVG_PCIE_TXTRANS_LATENCY, 
                (m_cycle - msg->m_txvc_start) * data_slots);
          }
          break; // one request pushed to dll per cycle
        }
      }
    }
  }
  // update round robin index
  m_txvc_rr_idx = (m_txvc_rr_idx + 1) % m_vc_cnt;

}

// FIXME (Done)
// - Currently we are making flits as messages come.
// - However, it may be better to wait for comming messages to form a single flit
// - as it can save bandwidth.
// - On the otherhand, if implemented wrongly, it can lengthen the packet latency.
// FIXME 2 (Done)
// - Currently, only one message can fit into a flit slot.
// - However, for response messages, multiple messages can fit into a slot.
// - Need to impelement this part to save BW.
// FIXME 3
// - More cleaner, general implementation???
void pcie_ep_c::process_txdll() {
  message_s* msg;
  flit_s* new_flit = NULL;
  int max_msg_per_flit = *KNOB(KNOB_PCIE_MAX_MSG_PER_FLIT);

  if (m_cur_flit) {
    m_flit_wait_cycle++;
  }

  // replay buffer is full
  if (m_txreplay_cap == (int)m_txreplay_buff.size()) {
    return;
  }

  for (; m_slot_cnt < max_msg_per_flit; ) {
    // dll q empty
    if ((int)m_txdll_q.size() == 0) {
      break;
    }

    msg = m_txdll_q.front();
/* if (msg->m_txtrans_end > m_cycle) { // message not ready */
/* break; */
    if (m_flit_ndr_cnt == 2 && msg->m_type == S2M_NDR) { // max ndr per flit
      break;
    } else if (m_flit_drs_cnt == 3 && msg->m_type == S2M_DRS) { // max drs per flit
      break;
    } else { // message ready
      m_txdll_q.pop_front();

      // make a flit if there is none
      if (m_cur_flit == NULL) {
        new_flit = m_flit_pool->acquire_entry(m_simBase);
        init_new_flit(new_flit, *KNOB(KNOB_PCIE_FLIT_BITS));
        m_cur_flit = new_flit;
      }
      m_cur_flit->m_msgs.push_back(msg);

      // FIXME : Currently, there is no specification about instruction offloading
      // Just assume that uops takes a single slot
      if (msg->m_type == M2S_REQ  || msg->m_type == M2S_RWD || 
          msg->m_type == M2S_DATA || msg->m_type == S2M_DATA ||
          msg->m_type == M2S_UOP || msg->m_type == S2M_UOP) { // takes one slot
        m_slot_cnt++;
      } else { // multiple messages fit in a slot
        if (m_slot_ndr_cnt == 0 && m_slot_drs_cnt == 0) {
          m_slot_cnt++;
        }

        if (msg->m_type == S2M_NDR) {
          m_slot_ndr_cnt++;
          m_flit_ndr_cnt++;
        } else if (msg->m_type == S2M_DRS) { // S2M_DRS
          m_slot_drs_cnt++;
          m_flit_drs_cnt++;
        }

        if ((m_slot_ndr_cnt == 2 && m_slot_drs_cnt == 1) ||
            (m_slot_ndr_cnt == 0 && m_slot_drs_cnt == 3) ||
            (m_slot_ndr_cnt == 2 && m_slot_drs_cnt == 0)) {
          m_slot_ndr_cnt = 0;
          m_slot_drs_cnt = 0;
        }
      }
    }
  }

  // insert to replay buffer if a new flit is made
  if (m_cur_flit && 
      ((m_flit_wait_cycle >= m_max_flit_wait) || (m_slot_cnt == max_msg_per_flit))) {
    m_cur_flit->m_txdll_end = m_cycle + *KNOB(KNOB_PCIE_TXDLL_LATENCY);
    m_txreplay_buff.push_back(m_cur_flit);

    m_cur_flit = NULL;
    m_flit_wait_cycle = 0;

    m_flit_ndr_cnt = 0;
    m_flit_drs_cnt = 0;
    m_slot_ndr_cnt = 0;
    m_slot_drs_cnt = 0;

    m_slot_cnt = 0;
  }
}

void pcie_ep_c::process_txphys() {
  // pop replay buffer entries that the peers received
  refresh_replay_buffer();

  if (!m_peer_ep->phys_layer_full()) {
    for (auto cur_flit : m_txreplay_buff) {
      if (cur_flit->m_phys_sent) {
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
        for (auto msg : cur_flit->m_msgs) {
          STAT_EVENT(PCIE_TXDLL_BASE);
          STAT_EVENT_N(AVG_PCIE_TXDLL_LATENCY, m_cycle - msg->m_txdll_start);
        }

        // update goodput related stats
        int good_bits = 0;
        for (auto msg : cur_flit->m_msgs) {
          good_bits += msg->m_bits;
        }
        STAT_EVENT_N(PCIE_GOODPUT_BASE, *KNOB(KNOB_PCIE_FLIT_BITS));
        STAT_EVENT_N(AVG_PCIE_GOODPUT, good_bits);

        // update phys latency related stats
        STAT_EVENT(PCIE_FLIT_BASE);
        STAT_EVENT_N(AVG_PCIE_PHYS_LATENCY, phys_finished - cur_flit->m_txdll_end);

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
      parse_and_insert_flit(flit);
    } else {
      break;
    }
  }
}

void pcie_ep_c::process_rxdll() {
  return;
}

void pcie_ep_c::process_rxtrans() {
  return;
}

int pcie_ep_c::get_rxvc_bw() {
  return m_rxvc_bw;
}

void pcie_ep_c::print_ep_info() {
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    std::cout << "======== TXVC[" << ii << "]" << std::endl;
    for (auto msg : m_txvc_buff[ii]) {
      msg->print();
      std::cout << std::endl;
    }
  }

  std::cout << "======== TXDLL" << std::endl;
  for (auto msg : m_txdll_q) {
    msg->print();
    std::cout << std::endl;
  }

  std::cout << "======= Cur Req" << std::endl;
  if (m_cur_flit) {
    m_cur_flit->print();
  }

  std::cout << "======= Replay buff" << std::endl;
  for (auto flit : m_txreplay_buff) {
    flit->print();
  }

  std::cout << "======= RX Physical" << std::endl;
  for (auto flit : m_rxphys_q) {
    flit->print();
  }

  for (int ii = 0; ii < m_vc_cnt; ii++) {
    std::cout << "======== RXVC[" << ii << "]" << std::endl;
    for (auto msg : m_rxvc_buff[ii]) {
      msg->print();
      std::cout << std::endl;
    }
  }
}

} // namespace CXL

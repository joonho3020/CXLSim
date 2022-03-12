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
#include "pcie_vcbuff.h"

#include "utils.h"
#include "all_knobs.h"
#include "all_stats.h"
#include "assert_macros.h"

namespace cxlsim {

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

  float freq = *KNOB(KNOB_CLOCK_IO);
  int lanes = *KNOB(KNOB_PCIE_LANES);
  int flit_bits = *KNOB(KNOB_PCIE_FLIT_BITS);
  m_phys_latency = 
    static_cast<Counter>(flit_bits / (lanes * m_perlane_bw) * freq);
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

  int tx_channel_cap = *KNOB(KNOB_PCIE_TXVC_CAPACITY);
  int rx_channel_cap = *KNOB(KNOB_PCIE_RXVC_CAPACITY);
  int tx_flitbuff_cap = *KNOB(KNOB_PCIE_TXFLITBUFF_CAPACITY);
  int rx_flitbuff_cap = *KNOB(KNOB_PCIE_RXFLITBUFF_CAPACITY);

  m_txvc->init(/* tx? */true,  m_master, 
                m_msg_pool, m_slot_pool, m_flit_pool, 
                tx_channel_cap, tx_flitbuff_cap);
  m_rxvc->init(/* tx? */false, m_master, 
                m_msg_pool, m_slot_pool, m_flit_pool,
                rx_channel_cap, rx_flitbuff_cap);
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
  assert(!phys_layer_full());
  m_rxphys_q.push_back(flit);
}

bool pcie_ep_c::has_free_rxvc(int vc_id) {
  return (m_rxvc->full(vc_id) == false);
}

//////////////////////////////////////////////////////////////////////////////
// private

Counter pcie_ep_c::get_phys_latency() {
  return m_phys_latency;
}

void pcie_ep_c::refresh_replay_buffer() {
  while (m_txreplay_buff.size()) {
    flit_s* flit = m_txreplay_buff.front();

    // if the flit is send && the flit is received by the peer
    if (flit->m_phys_sent && flit->m_phys_done <= m_cycle) {
      m_txreplay_buff.pop_front();
    } else {
      break;
    }
  }
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
  message_s* new_msg;
  int channel = m_txvc->get_channel(cxl_req);;

  if (m_txvc->full(channel) || m_txvc->flit_full()) {
    return false;
  } else {
    assert(m_txvc->free(channel) > 0);
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
    STAT_EVENT(PCIE_RXTRANS_BASE);
    STAT_EVENT_N(AVG_PCIE_RXTRANS_LATENCY, (m_cycle - msg->m_rxvc_insert_start));

/* if (msg->is_wdata_msg()) */
/* assert(msg->m_arrived_child == *KNOB(KNOB_PCIE_SLOTS_PER_FLIT)); */

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
      success &= check_peer_credit(msg);
    }
  }
  return success;
}

//////////////////////////////////////////////////////////////////////////////

void pcie_ep_c::process_txtrans() {
  m_txvc->generate_flits();
  m_txvc->run_a_cycle();
}

void pcie_ep_c::process_txdll() {
  int cnt = 0;
  while (m_txreplay_cap > (int)m_txreplay_buff.size()) {
    flit_s* flit = m_txvc->peek_flit();

    if (flit != NULL) {
      bool fctrl_success = check_peer_credit(flit);
      if (fctrl_success) {
        flit->m_txreplay_insert_start = m_cycle;
        flit->m_txreplay_insert_done = m_cycle + *KNOB(KNOB_PCIE_TXDLL_LATENCY);

        m_txreplay_buff.push_back(flit);
        m_txvc->pop_flit();
        cnt++;

        // update stats
        for (auto slot : flit->m_slots) {
          for (auto msg : slot->m_msgs) {
            STAT_EVENT(PCIE_TXTRANS_BASE);
            if (msg->m_data) {
              STAT_EVENT_N(AVG_PCIE_TXTRANS_LATENCY, 
                  (m_cycle - msg->m_parent->m_txvc_insert_start));
            } else {
              STAT_EVENT_N(AVG_PCIE_TXTRANS_LATENCY, 
                  (m_cycle - msg->m_txvc_insert_start));
            }
          }
        }
      }
    } else {
      break;
    }

    if (cnt == *KNOB(KNOB_PCIE_REPLAY_BW)) {
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
      } else if (cur_flit->m_txreplay_insert_done <= m_cycle) {
        // - packets are sent serially so transmission starts only after
        //   the previous packet finished physical layer transmission
        Counter lat = get_phys_latency() 
                      + 2*(*KNOB(KNOB_PCIE_ARBMUX_LATENCY)); // tx & rx
        Counter start_cyc = std::max(m_prev_txphys_cycle, m_cycle);
        Counter phys_finished = start_cyc + lat;

        m_prev_txphys_cycle = phys_finished;
        cur_flit->m_phys_start = start_cyc;
        cur_flit->m_phys_done = phys_finished;
        cur_flit->m_rxdll_done = phys_finished + *KNOB(KNOB_PCIE_RXDLL_LATENCY);
        cur_flit->m_phys_sent = true;

        // push to peer endpoint physical
        m_peer_ep->insert_phys(cur_flit);

        // update dll stats
        STAT_EVENT(PCIE_TXDLL_BASE);
        STAT_EVENT_N(AVG_PCIE_TXDLL_LATENCY,
                    (m_cycle - cur_flit->m_txreplay_insert_start));

        // update goodput related stats
        STAT_EVENT_N(PCIE_GOODPUT_BASE, *KNOB(KNOB_PCIE_FLIT_BITS));
        STAT_EVENT_N(AVG_PCIE_GOODPUT, cur_flit->m_bits);

        break;
      }
    }
  }
}

void pcie_ep_c::process_rxphys() {
  while (m_rxphys_q.size()) {
    flit_s* flit = m_rxphys_q.front();

    // finished physical layer & rx dll layer
    if (flit->m_rxdll_done <= m_cycle) {
      m_rxphys_q.pop_front();

      STAT_EVENT(PCIE_FLIT_BASE);
      STAT_EVENT_N(AVG_PCIE_PHYS_LATENCY, (flit->m_phys_done - flit->m_phys_start));
      STAT_EVENT(PCIE_RXDLL_BASE);
      STAT_EVENT_N(AVG_PCIE_RXDLL_LATENCY, (m_cycle - flit->m_phys_done));

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

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
 * File         : pcie_endpoint.h
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: cxl_t3.h 867 2021-10-08 02:28:12Z kacear $:
 * Description  : PCIe endpoint device
 *********************************************************************************************/

#ifndef PCIE_EP_H
#define PCIE_EP_H


#include <list>
#include <deque>

#include "packet_info.h"
#include "cxlsim.h"

namespace cxlsim {

class pcie_ep_c {
public:
  /**
   * Constructor
   */
  pcie_ep_c(cxlsim_c* simBase);

  /**
   * Destructor
   */
  virtual ~pcie_ep_c();

  /**
   * Initialize PCIe endpoint
   */
  void init(int id, bool master, pool_c<message_s>* msg_pool, 
            pool_c<slot_s>* slot_pool,
            pool_c<flit_s>* flit_pool, pcie_ep_c* peer);

  /**
   * Tick a cycle
   */
  virtual void run_a_cycle(bool pll_locked);

  /**
   * Checks if rx physical layer of the peer is full
   */
  bool phys_layer_full(void);

  /**
   * Receive packet from transmit side & put in rx physical q
   */
  void insert_phys(flit_s* flit);

  bool has_free_rxvc(int vc_id);

  /**
   * Print for debugging
   */
  void print_ep_info();

private:
  pcie_ep_c(); // do not implement

  /**
   * Gets cycles required to transfer the packet over physical layer
   */
  Counter get_phys_latency();

  /**
   * Checks & updates the state of entries if the flit is received by the peer
   */
  void refresh_replay_buffer(void);

  /*
   * Release msg
   */
  void release_msg(message_s* msg);

  /**
   * Checks if the peer has enough rxvc entries left for this message type
   * (for flow control)
   */
  bool check_peer_credit(message_s* msg);
  bool check_peer_credit(flit_s* flit);

protected:
  /**
   * Start PCIe transaction by inserting requests
   */
  virtual void start_transaction();

  /**
   * Push request to TX VC buffer
   */
  bool push_txvc(cxl_req_s* mem_req);

  /**
   * End PCIe transaction by pulling requests
   */
  virtual void end_transaction();

  /**
   * Pull request from RX VC buffer
   */
  cxl_req_s* pull_rxvc();

  // PCIE TX layer related
  void process_txtrans();
  void process_txdll();
  void process_txphys();

  // PCIE RX layer related
  void process_rxphys();
  void process_rxdll();
  void process_rxtrans();

  int get_rxvc_bw();

private:
  int m_id; /**< unique id of each endpoint */
  bool m_master; /**< endpoint is masterside when true */
  pool_c<message_s>* m_msg_pool; /**< message pool */
  pool_c<slot_s>* m_slot_pool;
  pool_c<flit_s>* m_flit_pool; /**< flit pool */

  int m_lanes; /**< PCIe lanes connected to endpoint */
  float m_perlane_bw; /**< PCIe per lane BW in GT/s */
  Counter m_prev_txphys_cycle; /**< finish cycle of previously sent packet */

  int m_rxvc_bw; /**< VC buffer BW */
  int m_txreplay_cap; /**< replay buffer capacity */
  std::list<flit_s*> m_txreplay_buff; /**< replay buffer */

  int m_phys_cap; /**< maximum numbers of packets in physical layer q */
  std::list<flit_s*> m_rxphys_q; /**< physical layer receive queue */
  Counter m_phys_latency;

  vc_buff_c* m_txvc;
  vc_buff_c* m_rxvc;

public:
  pcie_ep_c* m_peer_ep; /**< endpoint connected to this endpoint */
  cxlsim_c* m_simBase; /**< simulation base */
  Counter m_cycle; /**< PCIe clock cycle */
};

} // namespace CXL
#endif //PCIE_EP_H

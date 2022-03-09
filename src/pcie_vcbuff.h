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
 * Date         : 2/15/2022
 * SVN          : $Id: pcie_vcbuff.h 867 2021-10-08 02:28:12Z kacear $:
 * Description  : PCIe virtual channel buffer
 *********************************************************************************************/

#ifndef PCIE_VCBUFF_H
#define PCIE_VCBUFF_H

#include <list>
#include <deque>

#include "packet_info.h"
#include "cxlsim.h"

namespace cxlsim {

class vc_buff_c {
public:
  vc_buff_c(cxlsim_c* simBase); /**< constructor */
  void init(bool is_tx, bool is_master, 
            pool_c<message_s>* msg_pool, 
            pool_c<slot_s>* slot_pool,
            pool_c<flit_s>* flit_pool,
            int channel_cap,
            int flitbuff_cap);
  bool full(int vc_id); /**< true if vc_id is full, otherwise false */
  bool flit_full();
  bool empty(int vc_id); /**< true if vc_id is empty, otherwise false */
  int free(int vc_id); /**< number of free entries */
  int get_channel(cxl_req_s* req); /**< get channel id of request */
  void insert(cxl_req_s* req); /**< insert request into vc buffer */
  flit_s* peek_flit(); 
  void pop_flit(); /**< pop a flit from m_flit_buff */
  message_s* pull_msg(int vc_id); /**< pull msg from rxvc */
  void receive_flit(flit_s* flit); /**< receive flit from rxphys */
  void run_a_cycle(); /**< run a cycle */
  void generate_flits(); /**< look at vc buffers and generate a flit */
  void print();

private:
  void insert_channel(int vc_id, message_s* msg);
  void remove_msg(message_s* msg);
  void release_flit(flit_s* flit);
  void release_slot(slot_s* slot);
  void release_msg(message_s* msg);
  message_s* acquire_message(int channel, cxl_req_s* req);
  slot_s* acquire_slot(); /**< acquire slot from slot pool */
  flit_s* acquire_flit(); /**< acquire slot from flit pool */
  slot_s* generate_hslot(std::list<message_s*>& msgs); /**< generate header slot */
  slot_s* generate_gslot(std::list<message_s*>& msgs, flit_s* flit); /**< generate general slot */
  void generate_new_flit(std::list<message_s*>& msgs); /**< generate new flit */

  bool check_valid_header(slot_s* slot, message_s* msg); /**< check limit conditions for header flit */
  bool check_valid_general(slot_s* slot, message_s* msg, flit_s* flit); /**< check limit conditions for general flit */
  void add_data_slots_and_insert(flit_s* flit);
  void add_data_slots_and_insert(flit_s* flit, slot_s* slot);
  void insert_data_slots(flit_s* flit, std::list<slot_s*>& data_slots);

  void forward_progress_check();

public:
  static int m_msg_uid;
  static int m_slot_uid;
  static int m_flit_uid;

private:
  pool_c<message_s>* m_msg_pool;
  pool_c<slot_s>* m_slot_pool;
  pool_c<flit_s>* m_flit_pool;

  std::list<message_s*> m_msg_buff;
  std::list<flit_s*> m_flit_buff;

  int m_channel_cnt[MAX_CHANNEL];
  int m_channel_cap; /**< channel capacity */
  int m_flitbuff_cap;

  int m_hslot_msg_limit[MAX_MSG_TYPES]; /**< limit conditions */
  int m_gslot_msg_limit[MAX_MSG_TYPES];
  int m_flit_msg_limit[MAX_MSG_TYPES];

  bool m_istx; /** tx vc buffer */
  bool m_master; /**< master part */

  Counter m_cycle;
  cxlsim_c* m_simBase;

#ifdef DEBUG
public:
  Counter m_in_flight_reqs;
  Counter get_in_flight_reqs();
#endif
};

} // namespace CXL
#endif //PCIE_VCBUFF_H

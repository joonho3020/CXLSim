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
 * File         : pcie_rc.cc
 * Author       : Joonho
 * Date         : 10/10/2021
 * SVN          : $Id: pcie_rc.h 867 2021-10-10 02:28:12Z kacear $:
 * Description  : PCIe root complex
 *********************************************************************************************/

#include <cassert>
#include <iostream>
#include <vector>

#include "all_knobs.h"
#include "pcie_rc.h"

namespace cxlsim {

pcie_rc_c::pcie_rc_c(cxlsim_c* simBase) 
  : pcie_ep_c(simBase) {
  m_pending_size = *KNOB(KNOB_PCIE_INSERTQ_SIZE);
}

pcie_rc_c::~pcie_rc_c() {
}

void pcie_rc_c::run_a_cycle(bool pll_locked) {
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

void pcie_rc_c::start_transaction() {
  int cnt = 0;
  std::vector<cxl_req_s*> tmp_list;
  for (auto req : m_pending_req) {
    bool success = push_txvc(req);
    if (success) {
      tmp_list.push_back(req);
      cnt++;
    }
    if (cnt == *KNOB(KNOB_PCIE_TXVC_BW) || !success) {
      break;
    }
  }

  for (auto iter = tmp_list.begin(), end = tmp_list.end(); iter != end; ++iter) {
    m_pending_req.remove(*iter);
  }
}

void pcie_rc_c::end_transaction() {
  while (1) {
    cxl_req_s* req = pull_rxvc();
    if (!req) {
      break;
    } else {
      m_done_req.push_back(req);
    }
  }
}

void pcie_rc_c::insert_request(cxl_req_s* req) {
  assert((int)m_pending_req.size() < m_pending_size);
  m_pending_req.push_back(req);
}

bool pcie_rc_c::rootcomplex_full() {
  if ((int)m_pending_req.size() >= m_pending_size)
    return true;
  else
    return false;
}

cxl_req_s* pcie_rc_c::pop_request() {
  if (m_done_req.empty()) {
    return NULL;
  } else {
    cxl_req_s* req = m_done_req.front();
    m_done_req.pop_front();
    return req;
  }
}

void pcie_rc_c::print_rc_info() {
  std::cout << "-------------- Root Complex ------------------" << std::endl;
  print_ep_info();

/* std::cout << "pending q" << ": "; */
/* for (auto req : m_pending_req) { */
/* std::cout << std::hex << req->m_addr << " ; "; */
/* } */

  std::cout << "done q" << ": ";
  for (auto req : m_done_req) {
    std::cout << std::hex << req->m_addr << " ; ";
  }
  std::cout << std::endl;
}

}

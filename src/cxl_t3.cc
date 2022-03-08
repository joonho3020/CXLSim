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
 * File         : cxl_t3.h
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: cxl_t3.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : CXL type 3 device
 *********************************************************************************************/

#include <iostream>
#include <list>

#include "cxl_t3.h"
#include "pcie_rc.h"
#include "cxlsim.h"

#include "all_knobs.h"
#include "statistics.h"

#include "ramulator/src/Request.h"

namespace cxlsim {

cxlt3_c::cxlt3_c(cxlsim_c* simBase) 
  : pcie_ep_c(simBase),
    m_mxp_requestsInFlight(0),
    m_ramu_wrapper(NULL),
    m_read_cb_func(
      std::bind(&cxlt3_c::readComplete, this, std::placeholders::_1)),
    m_write_cb_func(std::bind(&cxlt3_c::writeComplete, this,
                            std::placeholders::_1)) {
  // init queues
  m_pending_req = new list<cxl_req_s*>();

  // init ramulator
  std::string config_file(*KNOB(KNOB_RAMULATOR_CONFIG_FILE));
  configs.parse(config_file);
  configs.set_core_num(*KNOB(KNOB_NUM_SIM_CORES));

  m_ramu_wrapper = new ramulator::CXLRamulatorWrapper(
    configs, *KNOB(KNOB_RAMULATOR_CACHELINE_SIZE),
    *KNOB(KNOB_STATISTICS_OUT_DIRECTORY));

  // init others
  m_cycle_internal = 0;
}

cxlt3_c::~cxlt3_c() {
  m_ramu_wrapper->finish();
  delete m_ramu_wrapper;
  delete m_pending_req;
}

void cxlt3_c::run_a_cycle(bool pll_locked) {
  // send response
  process_txphys();
  process_txdll();
  process_txtrans();
  start_transaction();

  // process memory requests
  process_pending_req();

  // receive memory request
  end_transaction(); 
  process_rxtrans();
  process_rxdll();
  process_rxphys();

  m_cycle++;
}

void cxlt3_c::run_a_cycle_internal(bool pll_locked) {
  m_ramu_wrapper->tick();
  m_cycle_internal++;
}

// for requests finished from ramulator, send the response back to 
// the root complex
void cxlt3_c::start_transaction() {
  int cnt = 0;
  vector<cxl_req_s*> tmp_list;
  for (auto req : m_mxp_resp_queue) {
    bool success = push_txvc(req);
    if (success) {
      tmp_list.push_back(req);
      cnt++;
    }
    if (cnt == *KNOB(KNOB_PCIE_TXVC_BW) || !success) {
      break;
    }
  }

  for (auto I = tmp_list.begin(), end = tmp_list.end(); I != end; ++I) {
    m_mxp_resp_queue.remove(*I);
  }
}

// transactions ends in the viewpoint of RC
// read messages from the rx vc & insert them into the dram pending queue
void cxlt3_c::end_transaction() {
  while (1) {
    cxl_req_s* req = pull_rxvc();
    if (!req) {
      break;
    } else {
      m_pending_req->push_back(req);
    }
  }
}

// insert requests in to the ramulator
void cxlt3_c::process_pending_req() {
  std::vector<cxl_req_s*> tmp_list;
  for (auto I = m_pending_req->begin(); I != m_pending_req->end(); I++) {
    cxl_req_s* req = *I;
    if (push_ramu_req(req)) {
      tmp_list.push_back(req);
    }
  }

  for (auto I = tmp_list.begin(), end = tmp_list.end(); I != end; ++I) {
    m_pending_req->remove(*I);
  }
}

bool cxlt3_c::push_ramu_req(cxl_req_s* req) {
  bool is_write = req->m_write;
  auto req_type = (is_write) ? ramulator::Request::Type::WRITE
                             : ramulator::Request::Type::READ;
  auto cb_func = (is_write) ? m_write_cb_func : m_read_cb_func;
  long addr = static_cast<long>(req->m_addr);

  ramulator::Request ramu_req(addr, req_type, cb_func, req->m_id);
  bool accepted = m_ramu_wrapper->send(ramu_req);

  if (accepted) {
    if (is_write) {
      // FIXME : fix this it req->m_id
      m_mxp_writes[ramu_req.addr].push_back(req);
    } else {
      m_mxp_reads[ramu_req.addr].push_back(req);
    }

    ++m_mxp_requestsInFlight;
    return true;
  } else {
    return false;
  }
}

// ramulator read callback
void cxlt3_c::readComplete(ramulator::Request &ramu_req) {
  if (*KNOB(KNOB_DEBUG_CALLBACK)) {
    printf("CXL RAM read callback done: 0x%lx\n", ramu_req.addr);
  }

  auto &req_q = m_mxp_reads.find(ramu_req.addr)->second;
  cxl_req_s *req = req_q.front();
  req_q.pop_front();
  if (!req_q.size()) m_mxp_reads.erase(ramu_req.addr);

  --m_mxp_requestsInFlight;
  m_mxp_resp_queue.push_back(req);
}

// ramulator write callback
void cxlt3_c::writeComplete(ramulator::Request &ramu_req) {
  if (*KNOB(KNOB_DEBUG_CALLBACK)) {
    printf("CXL RAM write callback done: 0x%lx\n", ramu_req.addr);
  }

  auto &req_q = m_mxp_writes.find(ramu_req.addr)->second;
  cxl_req_s *req = req_q.front();
  req_q.pop_front();
  if (!req_q.size()) m_mxp_writes.erase(ramu_req.addr);

  --m_mxp_requestsInFlight;
  m_mxp_resp_queue.push_back(req);
}

// print for debugging
void cxlt3_c::print_cxlt3_info() {
  std::cout << "-------------- mxp ------------------" << std::endl;
  print_ep_info();

  std::cout << "pending q" << ": ";
  for (auto req : *m_pending_req) {
    std::cout << std::hex << req->m_addr << " ; ";
  }

  std::cout << m_mxp_requestsInFlight << std::endl;

  std::cout << "Read q" << std::endl;
  for (auto iter : m_mxp_reads) {
    auto addr = iter.first;
    auto req_q = iter.second;
    std::cout << "Addr: " << std::hex << addr << ": ";
    for (auto req : req_q)  {
      std::cout << req << "; ";
    }
    std::cout << std::endl;
  }

  std::cout << "Write q" << std::endl;
  for (auto iter : m_mxp_writes) {
    auto addr = iter.first;
    auto req_q = iter.second;
    std::cout << "Addr: " << std::hex << addr << ": ";
    for (auto req : req_q)  {
      std::cout << req << "; ";
    }
    std::cout << std::endl;
  }

/* for (auto iter : *m_pending_req) { */
/* auto addr = iter->m_addr; */
/* std::cout << "Addr: " << std::hex << addr << ": "; */
/* } */
  std::cout << std::endl;
}

}

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
 * File         : core.cc
 * Author       : Joonho
 * Date         : 12/3/2021
 * SVN          : $Id: core.cc 867 2021-12-03 02:28:12Z kacear $:
 * Description  : core for callback test
 *********************************************************************************************/

#include <cassert>
#include <iostream>

#include "all_knobs.h"
#include "cxlsim.h"
#include "assert_macros.h"
#include "core.h"

namespace cxlsim {

//////////////////////////////////////////////////////////////////////////////

core_req_s::core_req_s() {
  init();
}

core_req_s::core_req_s(Addr addr, bool write, bool uop, int type) {
  m_addr = addr;
  m_write = write;
  m_isuop = uop;
  m_type = -1;
}

void core_req_s::init() {
  m_addr = 0;
  m_write = false;
  m_isuop = false;
  m_type = -1;
}

//////////////////////////////////////////////////////////////////////////////

core_c::core_c(cxlsim_c* simBase) {
  m_simBase = simBase;
  m_mem_insert_reqs = 0;
  m_mem_return_reqs = 0;
  m_uop_insert_reqs = 0;
  m_uop_return_reqs = 0;
  m_cycle = 0;
  m_unique_num = 0;

  callback_t *mem_callback = 
    new Callback<core_c, void, Addr, bool, void*> (&(*this),  &core_c::core_mem_callback);
  callback_t *uop_callback =
    new Callback<core_c, void, Addr, bool, void*> (&(*this), &core_c::core_uop_callback);

  m_simBase->register_memreq_callback(mem_callback);
  m_simBase->register_uopreq_callback(uop_callback);
}

core_c::~core_c() {
  delete m_simBase;
}

void core_c::set_tracefile(std::string filename) {
  m_tracefilename = filename;
}

std::pair<int, int> core_c::set_uop_type(int type) {
  assert(type >= 2);

  int uop_type; 
  int mem_type;
  switch (type) {
    case (2): // IADD
      uop_type = 7;
      mem_type = 0;
      break;
    case(3): //IMEM_LD
      uop_type = 6;
      mem_type = 1;
      break;
    case(4): //IMEM_ST
      uop_type = 6;
      mem_type = 2;
      break;
    default:
      assert(0);
  }

  return {uop_type, mem_type};
}

void core_c::insert_request(Addr addr, int type) {
  bool uop = (type >= 2);
  bool write = (type == 1);

  core_req_s* new_req = new core_req_s(addr, write, uop, type);

  bool success = false;
  if (!uop) {
    success = m_simBase->insert_mem_request(addr, write, (void*)new_req);
  } else {
    // as an example, insert a uop request that is dependent on the
    // previous uop
    std::vector<std::pair<Counter, int>> src_uop_list;
    src_uop_list.push_back({m_unique_num, 1});

    int latency = 3; // just an example
    int core_id = 0;
    auto uop_info = set_uop_type(type);
    success = m_simBase->insert_uop_request((void*)new_req, core_id, uop_info.first, 
                                            uop_info.second, addr, 
                                            ++m_unique_num, latency, src_uop_list);
  }

  if (!success) {
    m_pending_q.push_back(new_req);
  }

  // debug messages
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== insert core req =================================" << std::endl;
    std::cout << addr << " " << write << " " << " " << (uop ? 1 : 0) << " " << new_req << std::endl;
  }

  if (uop) m_uop_insert_reqs++;
  else m_mem_insert_reqs++;
}

void core_c::run_a_cycle(bool pll_locked) {
  // if the pending_q is not empty insert one req into cxl every cycle
  if (!m_pending_q.empty()) {
    core_req_s* req = m_pending_q.front();
    bool success = false;
    if (req->m_isuop) {
      // as an example, insert a uop request that is dependent on the
      // previous uop
      std::vector<std::pair<Counter, int>> src_uop_list;
      src_uop_list.push_back({m_unique_num, 1});

      int latency = 3; // just an example
      int core_id = 0;
      auto uop_info = set_uop_type(req->m_type);
      success = m_simBase->insert_uop_request((void*)req, core_id,
                                        uop_info.first, uop_info.second, 
                                        req->m_addr, ++m_unique_num, 
                                        latency, src_uop_list);
    } else {
      success = m_simBase->insert_mem_request(req->m_addr, req->m_write, 
                                              (void*)req);
    }

    if (success) {
      m_pending_q.pop_front();
    }
  }

  m_simBase->run_a_cycle(pll_locked);
  m_cycle++;
}

void core_c::run_sim() {
  std::ifstream file(m_tracefilename);

  // read traces
  Counter tot_reqs = 0;
  if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        Addr addr;
        int type;
        std::sscanf(line.c_str(), "%lld %d", &addr, &type);

        insert_request(addr, type);
        tot_reqs++;
      }
    file.close();
  }

  // run simulation
  while (m_mem_return_reqs + m_uop_return_reqs < tot_reqs) {
    run_a_cycle(false);
  }

  ASSERTM(m_mem_return_reqs == m_mem_insert_reqs, "Unit test failed : Some mem reqs not returning");
  ASSERTM(m_uop_return_reqs == m_uop_insert_reqs, "Unit test failed : Some uop reqs not returning");

  std::cout << "Unit test success" << std::endl;
}

void core_c::core_mem_callback(Addr addr, bool write, void *req) {
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== mem callback =================================" << std::endl;
    std::cout << m_mem_return_reqs << " " << addr << " " << write << " " << req << std::endl;
  }

  core_req_s* cur_req = static_cast<core_req_s*>(req);
  assert(addr == cur_req->m_addr);
  assert(write == cur_req->m_write);

  m_mem_return_reqs++;
  delete cur_req;
}

void core_c::core_uop_callback(Addr addr, bool write, void *req) {
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== uop callback =================================" << std::endl;
    std::cout << m_uop_return_reqs << " " << addr << " " << write << " " << req << std::endl;
  }

  core_req_s* cur_req = static_cast<core_req_s*>(req);
  assert(addr == cur_req->m_addr);
/* assert(write == cur_req->m_write); */

  m_uop_return_reqs++;
  delete cur_req;
}


} // namespace CXL

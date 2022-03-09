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
#include "core.h"

namespace cxlsim {

//////////////////////////////////////////////////////////////////////////////

core_req_s::core_req_s() {
  init();
}

core_req_s::core_req_s(Addr addr, bool write) {
  m_addr = addr;
  m_write = write;
}

void core_req_s::init() {
  m_addr = 0;
  m_write = false;
}

//////////////////////////////////////////////////////////////////////////////

core_c::core_c(cxlsim_c* simBase) {
  m_simBase = simBase;
  m_return_reqs = 0;
  m_insert_reqs = 0;
  m_cycle = 0;

  callback_t *trans_callback = 
    new Callback<core_c, void, Addr, bool, Counter, void*>(&(*this), &core_c::core_callback);

  m_simBase->register_callback(trans_callback);

#ifdef CXL_DEBUG
  m_in_flight_reqs = 0;
#endif
}

core_c::~core_c() {
  delete m_simBase;
}

void core_c::set_tracefile(std::string filename) {
  m_tracefilename = filename;
}

void core_c::insert_request(Addr addr, bool write) {
  core_req_s* new_req = new core_req_s(addr, write);
  m_pending_q.push_back(new_req);

  // debug messages
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== insert core req =================================" << std::endl;
    std::cout << m_insert_reqs << " " << addr << " " << write << " " << new_req << std::endl;
    m_insert_reqs++;
  }
}

void core_c::run_a_cycle(bool pll_locked) {
  // if the pending_q is not empty insert one req into cxl every cycle
#ifdef CXL_DEBUG
  if (!m_pending_q.empty() && m_simBase->get_in_flight_reqs() < 100) {
#else
  if (!m_pending_q.empty()) {
#endif
    core_req_s* req = m_pending_q.front();
    if(m_simBase->insert_request(req->m_addr, req->m_write, (void*)req)) {
      m_pending_q.pop_front();

#ifdef CXL_DEBUG
      m_input_req_cnt[req->m_addr]++;
      m_input_insert_cycle[req->m_addr] = m_cycle;
      m_in_flight_reqs++;
#endif

    }
  }
  m_simBase->run_a_cycle(pll_locked);

  m_cycle++;

#ifdef CXL_DEBUG
  std::cout << m_cycle << ": " << m_in_flight_reqs << "/"
                              << m_simBase->get_in_flight_reqs() << std::endl;
  assert(m_in_flight_reqs == m_simBase->get_in_flight_reqs());
  check_forward_progress();
#endif
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
        bool write;

        std::sscanf(line.c_str(), "%llu %d", &addr, &type);
        write = (type == 1);
        insert_request(addr, write);
        tot_reqs++;
      }
    file.close();
  }

  // run simulation
  while (m_return_reqs < tot_reqs) {
    run_a_cycle(false);
  }

#ifdef CXL_DEBUG
  for (auto ent : m_input_req_cnt) {
    auto addr = ent.first;
    auto cnt = ent.second;
    if (cnt != 0) {
      std::cout << "Addr: " << addr 
                << "Unreturned reqs: " <<  cnt << std::endl;
    }
  }
#endif

  std::cout << "Test passed" << std::endl;
}

#ifdef CXL_DEBUG
void core_c::check_forward_progress() {
  auto period = m_simBase->m_knobs->KNOB_FORWARD_PROGRESS_PERIOD->getValue();

  if (m_cycle % period != 0) {
    return;
  }

  for (auto ent : m_input_req_cnt) {
    auto addr = ent.first;
    auto cnt = ent.second;
    if (cnt) {
      std::cout << addr << " ";
    }
    std::cout << std::endl;
  }

  for (auto ent : m_input_insert_cycle) {
    auto addr = ent.first;

    auto cnt = m_input_req_cnt[addr];
    if (cnt == 0) continue;

    auto in_cycle = ent.second;
    if (m_cycle - in_cycle > period) {
      std::cout << "Forward progress limit at Addr: " << addr
                << " Insert cycle: " << in_cycle
                << " Cur cycle: " << m_cycle << std::endl;
      assert(0);
    }
  }
}
#endif

void core_c::core_callback(Addr addr, bool write, Counter req_id, void *req) {
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== core callback =================================" << std::endl;
    std::cout << m_return_reqs << " " << addr << " " << write << " " << req << std::endl;
  }

  core_req_s* cur_req = static_cast<core_req_s*>(req);

#ifdef CXL_DEBUG
  assert(addr == cur_req->m_addr);
  assert(write == cur_req->m_write);
  assert(--m_input_req_cnt[addr] >= 0);
  assert(--m_in_flight_reqs >= 0);
#endif

  m_return_reqs++;
  delete cur_req;
}

} // namespace CXL

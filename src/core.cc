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

#ifdef CXL_DEBUG
#include "cxl_t3.h"
#endif

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
    new Callback<core_c, void, Addr, bool, Counter, void*>
                (&(*this), &core_c::core_callback);

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

void core_c::insert_request(Addr addr, bool write, Counter cycle) {
  core_req_s* new_req = new core_req_s(addr, write);
  m_pending_q.push_back({new_req, cycle});

  // debug messages
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== insert core req =================================" << std::endl;
    std::cout << m_insert_reqs << " " << addr << " " << write << " " << new_req << std::endl;
    m_insert_reqs++;
  }
}

void core_c::run_a_cycle(bool pll_locked) {
  // if the pending_q is not empty insert one req into cxl every cycle
  if (!m_pending_q.empty()) {
    auto ent = m_pending_q.front();

    auto req = ent.first;
    auto cycle = ent.second;

#ifdef CXL_DEBUG
    if (cycle > m_cycle && m_simBase->get_in_flight_reqs() == 0) {
      assert(m_in_flight_reqs == 0);
      m_simBase->fast_forward(cycle - m_cycle);
      m_cycle = cycle;
    }
#endif

    if (cycle == m_cycle) {

      Counter req_id = 
        m_simBase->insert_request(req->m_addr, req->m_write, (void*)req);

      if (req_id != 0) {
        m_pending_q.pop_front();

#ifdef CXL_DEBUG
        m_in_flight_reqs++;
        m_in_flight_reqs_id[req_id] += 1;
#endif
      }
    }
  }

  m_simBase->run_a_cycle(pll_locked);
  m_cycle++;

#ifdef CXL_DEBUG
/* std::cout << m_cycle << ": " << m_in_flight_reqs << "/" */
/* << m_simBase->get_in_flight_reqs() << std::endl; */
/* assert(m_in_flight_reqs == m_simBase->get_in_flight_reqs()); */
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
        Counter cycle;
        bool write;

        std::sscanf(line.c_str(), "%llu %d %llu", &addr, &type, &cycle);
        write = (type == 1);
        insert_request(addr, write, cycle);
        tot_reqs++;
      }
    file.close();
  }

  // run simulation
  while (m_return_reqs < tot_reqs) {
    run_a_cycle(false);
  }
  std::cout << "Simulation ended" << std::endl;
}

#ifdef CXL_DEBUG
void core_c::check_forward_progress() {
  auto period = m_simBase->m_knobs->KNOB_FORWARD_PROGRESS_PERIOD->getValue();

  if (m_cycle % period != 0) {
    return;
  }

  if (m_simBase->m_mxp->check_ramulator_progress()) {
    std::cout << "Current cycle: " << m_cycle << std::endl;
    m_simBase->print();

    std::cout << "In flight requests not returned" << std::endl;
    for (auto x : m_in_flight_reqs_id) {
      if (x.second) std::cout << x.first << std::endl;
    }
    assert(0);
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
  assert(--m_in_flight_reqs >= 0);
  m_in_flight_reqs_id[req_id]--;
#endif

  m_return_reqs++;
  delete cur_req;
}

} // namespace CXL

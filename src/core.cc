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

  callback_t *trans_callback = 
    new Callback<core_c, void, Addr, bool, void*>(&(*this), &core_c::core_callback);

  m_simBase->register_callback(trans_callback);
}

core_c::~core_c() {
  delete m_simBase;
}

void core_c::set_tracefile(std::string filename) {
  m_tracefilename = filename;
}

void core_c::insert_request(Addr addr, bool write) {
  core_req_s* new_req = new core_req_s(addr, write);

  if (m_simBase->insert_request(addr, write, (void*)new_req)) {
    // do nothing
  } else {
    m_pending_q.push_back(new_req);
  }

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
    core_req_s* req = m_pending_q.front();
    if(m_simBase->insert_request(req->m_addr, req->m_write, (void*)req)) {
      m_pending_q.pop_front();
    }
  }

  m_simBase->run_a_cycle(pll_locked);
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

        std::sscanf(line.c_str(), "%llx %d", &addr, &type);
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
}

void core_c::core_callback(Addr addr, bool write, void *req) {
  if (m_simBase->m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
    std::cout << "======================== core callback =================================" << std::endl;
    std::cout << m_return_reqs << " " << addr << " " << write << " " << req << std::endl;
  }

  core_req_s* cur_req = static_cast<core_req_s*>(req);
  assert(addr == cur_req->m_addr);
  assert(write == cur_req->m_write);

  m_return_reqs++;
  delete cur_req;
}

} // namespace CXL

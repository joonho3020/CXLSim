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
 * File         : cxlsim.cc
 * Author       : Joonho
 * Date         : 10/10/2021
 * SVN          : $Id: cxlsim.cc 867 2021-10-10 02:28:12Z kacear $:
 * Description  : CXL simulation base class
 *********************************************************************************************/

#include <iostream>

#include "cxlsim.h"
#include "pcie_rc.h"
#include "cxl_t3.h"
#include "uop.h"
#include "packet_info.h"
#include "utils.h"
#include "statistics.h"
#include "all_knobs.h"
#include "all_stats.h"

namespace cxlsim {

#define GET_NEXT_CYCLE(domain)              \
  ++m_domain_count[domain];                 \
  m_domain_next[domain] = static_cast<int>( \
    1.0 * m_clock_lcm * m_domain_count[domain] / m_domain_freq[domain]);


int get_gcd(int a, int b) {
  if (b == 0) return a;

  return get_gcd(b, a % b);
}

int get_lcm(int a, int b) {
  return (a * b) / get_gcd(a, b);
}


///////////////////////////////////////////////////////////////////////////////
// public
///////////////////////////////////////////////////////////////////////////////

cxlsim_c::cxlsim_c() {
  // simulation related
  m_cycle = 0;

  // memory pool for packets
  m_uop_pool = new pool_c<uop_s>;
  m_req_pool = new pool_c<cxl_req_s>;
  m_msg_pool = new pool_c<message_s>;
  m_flit_pool = new pool_c<flit_s>;
  m_mem_trans_done_cb = NULL;
  m_uop_trans_done_cb = NULL;

  // clock domain
  m_clock_lcm = 1;
  m_domain_freq = new int[2];
  m_domain_count = new int[2];
  m_domain_next = new int[2];
  m_clock_internal = 0;
}

cxlsim_c::~cxlsim_c() {
  delete m_rc;
  delete m_mxp;
  delete m_req_pool;
  delete m_msg_pool;
  delete m_flit_pool;
  delete m_domain_freq;
  delete m_domain_count;
  delete m_domain_next;
}

void cxlsim_c::init(int argc, char** argv) {
  init_knobs(argc, argv);
  init_stats();
  init_sim_objects();
  init_clock_domain();
}

void cxlsim_c::register_memreq_callback(callback_t* fn) {
  m_mem_trans_done_cb = fn;
}

void cxlsim_c::register_uopreq_callback(callback_t* fn) {
  m_uop_trans_done_cb = fn;
}

// accept request from the outside simulator
bool cxlsim_c::insert_mem_request(Addr addr, bool write, void* req) {
  if (m_rc->rootcomplex_full()) {
    return false;
  } else {
    if (m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
      std::cout << "==== Insert req to MXP"
        << std::hex << " Addr: " << addr
        << std::dec << " write: " << write << std::endl;
    }

    // assign a new request
    cxl_req_s* new_req = m_req_pool->acquire_entry(this);
    new_req->init(addr, write, false, NULL, req);

    m_rc->insert_request(new_req);
    return true;
  }
}

bool cxlsim_c::insert_uop_request(void* req, int uop_type, int mem_type,
                          Addr addr, Counter unique_id, 
                          std::vector<std::pair<Counter, int>> src_uop_list) {
  if (m_rc->rootcomplex_full()) {
    return false;
  } else {
    if (m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
      std::cout << "==== Insert req to MXP"
        << std::hex << " uop_type: " << uop_type
        << std::dec << " mem_type: " << mem_type << std::endl;
    }

    // assign a new uop
    uop_s* new_uop = m_uop_pool->acquire_entry(this);
    new_uop->m_unique_num = unique_id;
    new_uop->m_uop_type = (Uop_Type)(uop_type);
    new_uop->m_mem_type = (Mem_Type)(mem_type);
    new_uop->m_addr = addr;
    new_uop->m_src_rdy = false;
    for (int ii = 0, src_cnt = 0; ii < (int)src_uop_list.size(); ii++) {
      Counter src_unique_id = src_uop_list[ii].first;
      Dep_Type dep_type = (Dep_Type)(src_uop_list[ii].second);

      if (m_uop_map.find(src_unique_id) == m_uop_map.end()) {
        continue;
      }
      new_uop->m_map_src_info[src_cnt] = {dep_type, m_uop_map[src_unique_id]};
      src_cnt++;
    }

    // update uop map table
    m_uop_map[unique_id] = new_uop;

    // assign a new request
    cxl_req_s* new_req = m_req_pool->acquire_entry(this);
    new_req->init(addr, false, true, new_uop, req);

    m_rc->insert_request(new_req);
    return true;
  }
}

void cxlsim_c::run_a_cycle(bool pll_locked) {
  // run root complex & memory expander
  // - from the viewpoint of the external simulator, the interconnect should 
  //   run_a_cycle whenever cxlsim_c::run_a_cycle is called
  m_mxp->run_a_cycle(pll_locked);
  m_rc->run_a_cycle(pll_locked);
  GET_NEXT_CYCLE(CLOCK_IO);

  // run dram inside the memory expander
  // - should run only when the timing is correct
  while (m_clock_internal <= m_domain_next[CLOCK_CXLRAM] &&
      m_domain_next[CLOCK_CXLRAM] < m_domain_next[CLOCK_IO]) {
    m_mxp->run_a_cycle_internal(pll_locked);
    GET_NEXT_CYCLE(CLOCK_CXLRAM);
  }

  // pull finished requests from the root complex
  while (1) {
    cxl_req_s* finished_req = m_rc->pop_request();
    if (finished_req == NULL) { // no finished request
      break;
    } else { // call the callback function & return to req pool
      request_done(finished_req);
    }
  }

  // update external clock 
  m_cycle++;

  // update internal clock
  m_clock_internal += static_cast<int>(1.0 * m_clock_lcm / 
                                       m_domain_freq[CLOCK_IO]);
  if (m_clock_internal == m_clock_lcm) {
    m_clock_internal = 0;
    for (int ii = 0; ii < 2; ii++) {
      m_domain_count[ii] = 0;
      m_domain_next[ii] = 0;
    }
  }

  // print messages for debugging
  if (m_knobs->KNOB_DEBUG_IO_SYS->getValue()) {
    std::cout << std::endl << "io cycle : " << std::dec << m_cycle << std::endl;
    m_mxp->print_cxlt3_info();
    m_rc->print_rc_info();
  }
}

void cxlsim_c::finalize() {
  // dump stats
  m_ProcessorStats->saveStats();
}

//////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////

void cxlsim_c::init_sim_objects() {
  // io devices
  m_rc = new pcie_rc_c(this);
  m_mxp = new cxlt3_c(this);

  //    init(id, master, msg_pool,   flit_pool,   peer)
  m_rc->init( 0, true,   m_msg_pool, m_flit_pool, m_mxp);
  m_mxp->init(1, false,  m_msg_pool, m_flit_pool, m_rc);
}

void cxlsim_c::init_knobs(int argc, char** argv) {
  // Create the knob managing class
  m_knobsContainer = new KnobsContainer();

  // Get a reference to the actual knobs for this component instance
  m_knobs = m_knobsContainer->getAllKnobs();

  // apply values from parameters file
  m_knobsContainer->applyParamFile("cxl_params.in");

  // apply the supplied command line switches
  char* pInvalidArgument = NULL;
  if (!m_knobsContainer->applyComandLineArguments(argc, argv,
                                                  &pInvalidArgument)) {
  }

  // save the states of all knobs to a file
  m_knobsContainer->saveToFile("cxl_params.out");
}

void cxlsim_c::init_stats() {
  // init core & processor stats
  m_coreStatsTemplate = new CoreStatistics(this);
  m_ProcessorStats = new ProcessorStatistics(this);

  // init all stats
  m_allStats = new all_stats_c(m_ProcessorStats);
  m_allStats->initialize(m_ProcessorStats, m_coreStatsTemplate);
}

void cxlsim_c::init_clock_domain() {
  const int domain_cnt = 2;
  float freq[domain_cnt];

  freq[0] = m_knobs->KNOB_CLOCK_IO->getValue();
  freq[1] = m_knobs->KNOB_CLOCK_CXLRAM->getValue();

  // allow only .x format
  for (int ii = 0; ii < 1; ++ii) {
    bool found = false;
    for (int jj = 0; jj < domain_cnt; ++jj) {
      int int_cast = static_cast<int>(freq[jj]);
      float float_cast = static_cast<float>(int_cast);
      if (freq[jj] != float_cast) {
        found = true;
        break;
      }
    }

    if (found) {
      for (int jj = 0; jj < domain_cnt; ++jj) {
        freq[jj] *= 10;
      }
    } else {
      break;
    }
  }

  m_domain_freq[0] = static_cast<int>(freq[0]);
  m_domain_freq[1] = static_cast<int>(freq[1]);

  m_clock_lcm = m_domain_freq[0];
  for (int ii = 1; ii < domain_cnt; ii++) {
    m_clock_lcm = get_lcm(m_clock_lcm, m_domain_freq[ii]);
  }
}

void cxlsim_c::request_done(cxl_req_s* req) {
  // call registered callback function if it has one
  if (m_uop_trans_done_cb && req->m_isuop) {
    if (m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
      std::cout << "CXL UOP Req Done: " << std::endl;
    }
    (*m_uop_trans_done_cb)(req->m_addr, req->m_write, req->m_req);
  }
  else if (m_mem_trans_done_cb) {
    assert(!req->m_isuop);

    if (m_knobs->KNOB_DEBUG_CALLBACK->getValue()) {
      std::cout << "CXL Req Done: "
                << std::hex << "Addr: " << req->m_addr << " " 
                << std::dec << "Write: " << req->m_write << " " << std::endl;
    }
    (*m_mem_trans_done_cb)(req->m_addr, req->m_write, req->m_req);
  }

  // release uop entry
  if (req->m_isuop) {
    uop_s* cur_uop = req->m_uop;
    Counter unique_num = cur_uop->m_unique_num;
    assert(m_uop_map.find(unique_num) != m_uop_map.end());

    // update uop table
    m_uop_map.erase(unique_num);

    // release entry
    req->m_uop->init();
    m_uop_pool->release_entry(cur_uop);
  }

  // release cxl request entry
  req->init();
  m_req_pool->release_entry(req);
}

} // namespace CXL

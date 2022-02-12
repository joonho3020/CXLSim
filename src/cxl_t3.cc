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
#include "cache.h"
#include "cxlsim.h"
#include "uop.h"

#include "all_knobs.h"
#include "statistics.h"
#include "utils.h"

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

  m_cache = new cache_c(m_simBase);

  init_ports();

  // init others
  m_cycle_internal = 0;
}

cxlt3_c::~cxlt3_c() {
  m_ramu_wrapper->finish();
  delete m_ramu_wrapper;
  delete m_pending_req;
  delete m_cache;
}

void cxlt3_c::init_cache() {
  m_cache->init_cache(*KNOB(KNOB_NDP_CACHE_SET),
                      *KNOB(KNOB_NDP_CACHE_ASSOC),
                      *KNOB(KNOB_CACHE_PORT_LAT));

  m_cache->init_mshr(*KNOB(KNOB_NDP_MSHR_ASSOC),
                     *KNOB(KNOB_NDP_MSHR_CAP));
}

void cxlt3_c::init_ports() {
  auto nop_port = new port_c(m_simBase);
  nop_port->init(*KNOB(KNOB_NOP_PORT_CNT), *KNOB(KNOB_NOP_PORT_LAT));
  m_ports.push_back(nop_port);

  auto iadd_port = new port_c(m_simBase);
  iadd_port->init(*KNOB(KNOB_IADD_PORT_CNT), *KNOB(KNOB_IADD_PORT_LAT));
  m_ports.push_back(iadd_port);

  auto imul_port = new port_c(m_simBase);
  imul_port->init(*KNOB(KNOB_IMUL_PORT_CNT), *KNOB(KNOB_IMUL_PORT_LAT));
  m_ports.push_back(imul_port);

  auto idiv_port = new port_c(m_simBase);
  idiv_port->init(*KNOB(KNOB_IDIV_PORT_CNT), *KNOB(KNOB_IDIV_PORT_LAT));
  m_ports.push_back(idiv_port);

  auto imisc_port = new port_c(m_simBase);
  imisc_port->init(*KNOB(KNOB_IMISC_PORT_CNT), *KNOB(KNOB_IMISC_PORT_LAT));
  m_ports.push_back(imisc_port);

  auto fadd_port = new port_c(m_simBase);
  fadd_port->init(*KNOB(KNOB_FADD_PORT_CNT), *KNOB(KNOB_FADD_PORT_LAT));
  m_ports.push_back(fadd_port);

  auto fmul_port = new port_c(m_simBase);
  fmul_port->init(*KNOB(KNOB_FMUL_PORT_CNT), *KNOB(KNOB_FMUL_PORT_LAT));
  m_ports.push_back(fmul_port);

  auto fdiv_port = new port_c(m_simBase);
  fdiv_port->init(*KNOB(KNOB_FDIV_PORT_CNT), *KNOB(KNOB_FDIV_PORT_LAT));
  m_ports.push_back(fdiv_port);

  auto fmisc_port = new port_c(m_simBase);
  fmisc_port->init(*KNOB(KNOB_FMISC_PORT_CNT), *KNOB(KNOB_FMISC_PORT_LAT));
  m_ports.push_back(fmisc_port);

  auto cache_port = new port_c(m_simBase);
  cache_port->init(*KNOB(KNOB_CACHE_PORT_CNT), *KNOB(KNOB_CACHE_PORT_LAT));
  m_ports.push_back(cache_port);
}

void cxlt3_c::run_a_cycle(bool pll_locked) {
  if (*KNOB(KNOB_DEBUG_UOP)) {
    print_cxlt3_uops();
  }

  if (*KNOB(KNOB_DEBUG_CACHE)) {
    m_cache->print();
  }

  // send response
  process_txphys();
  process_txdll();
  process_txtrans();
  start_transaction();

  // ports
  for (auto port : m_ports)
    port->run_a_cycle();

  // process ndp uops
  process_exec_queue();
  process_issue_queue();
  process_pending_uops();

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

void cxlt3_c::push_uop_direct(cxl_req_s* req) {
  m_pend_uop_queue.push_back(req);
}

// for requests finished from ramulator, send the response back to 
// the root complex
void cxlt3_c::start_transaction() {
  vector<cxl_req_s*> tmp_list;

  for (auto req : m_mxp_resp_queue) {
    if (push_txvc(req)) {
      tmp_list.push_back(req);
    } else {
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

    if (req->m_isuop) {
      m_pend_uop_queue.push_back(req);
      tmp_list.push_back(req);
    } else if (push_ramu_req(req)) {
      tmp_list.push_back(req);
    }
  }

  for (auto I = tmp_list.begin(), end = tmp_list.end(); I != end; ++I) {
    m_pending_req->remove(*I);
  }
}

bool cxlt3_c::check_ready(uop_s* cur_uop) {
  bool ready = true;

  for (int ii = 0; ii < cur_uop->m_src_cnt; ii++) {
    uop_s* src_uop = cur_uop->m_map_src_info[ii].m_uop;

    // check if the src uop is valid
    if ((src_uop == NULL) || (src_uop->m_valid == false) || 
        (src_uop->m_unique_num > cur_uop->m_unique_num)) {
      continue;
    }

    // check if src uop is ready
    if ((src_uop->m_done_cycle == 0) || (src_uop->m_done_cycle < m_cycle)) {
      if ((cur_uop->m_last_dep_exec == 0) || 
          (cur_uop->m_last_dep_exec < src_uop->m_done_cycle)) {
        cur_uop->m_last_dep_exec = src_uop->m_done_cycle;
      }
      ready = false;
      return false;
    }
  }

  cur_uop->m_src_rdy = ready;
  return ready;
}

void cxlt3_c::process_pending_uops() {
  std::vector<cxl_req_s*> tmp_list;
  for (auto it = m_pend_uop_queue.begin(); it != m_pend_uop_queue.end(); it++) {
    cxl_req_s* req = *it;

    auto cur_uop = req->m_uop;
    assert(cur_uop);

    bool success = false;
    if (check_ready(cur_uop)) { // check dependencies
      if (check_port(cur_uop)) { // check execution ports
        m_issue_queue.push_back(req);
        tmp_list.push_back(req);
        success = true;

        if (*KNOB(KNOB_DEBUG_MISC)) {
          std::cout << " ISSUE: ";
          cur_uop->print();
        }
      }
    } 

    if ((KNOB(KNOB_NDP_SCHEDULER)->getValue() == "in_order") &&
        success == false) {
      break;
    }
  }

  for (auto it = tmp_list.begin(), end = tmp_list.end(); it != end; ++it) {
    m_pend_uop_queue.remove(*it);
  }
}

// TODO : memory reqs that are prefetch requests?
void cxlt3_c::process_issue_queue() {
  int issue_cnt = 0;
  std::vector<cxl_req_s*> tmp_list;
  for (auto it = m_issue_queue.begin(); it != m_issue_queue.end(); it++) {
    auto req = *it;
    auto cur_uop = req->m_uop;
    
    if (cur_uop->m_mem_type == NOT_MEM) {
      cur_uop->m_exec_cycle = m_cycle;
      cur_uop->m_done_cycle = m_cycle + cur_uop->m_latency; // Clock Skew?

      tmp_list.push_back(req);
      m_exec_queue.push_back(req);

      issue_cnt++;
    } else { // memory instructions
      if (m_cache->lookup(cur_uop->m_addr)) { // cache hit
        cur_uop->m_exec_cycle = m_cycle;
        cur_uop->m_done_cycle = m_cycle + m_cache->get_lat();

        tmp_list.push_back(req);
        m_exec_queue.push_back(req);

        STAT_EVENT(CACHE_HIT_RATE_BASE);
        STAT_EVENT(CACHE_HIT_RATE);
      } else { // cache miss
        if (m_cache->is_first_miss(cur_uop->m_addr)) { // first miss to pfn
          if (m_cache->has_free_mshr() && push_ramu_req(req)) {
            cur_uop->m_exec_cycle = m_cycle;
            assert(m_cache->insert_mshr(req));
            tmp_list.push_back(req);

            STAT_EVENT(CACHE_HIT_RATE_BASE);
            STAT_EVENT(CACHE_MISS_RATE);
          }
        } else if (m_cache->insert_mshr(req)) { // insert req into mshr
          cur_uop->m_exec_cycle = m_cycle;
          tmp_list.push_back(req);

          STAT_EVENT(CACHE_HIT_RATE_BASE);
          STAT_EVENT(CACHE_MISS_RATE);
        }
      }
    }
  }

  for (auto it = tmp_list.begin(), end = tmp_list.end(); it != end; ++it) {
    m_issue_queue.remove(*it);
  }

  if (*KNOB(KNOB_DEBUG_MISC)) {
    std::cout << "COUNT_ISSUE : " << m_cycle << " : " << issue_cnt << std::endl;
  }
}

void cxlt3_c::process_exec_queue() {
  std::vector<cxl_req_s*> tmp_list;
  for (auto it = m_exec_queue.begin(); it != m_exec_queue.end(); it++) {
    auto req = *it;
    auto cur_uop = req->m_uop;

    if (cur_uop->m_done_cycle <= m_cycle) {
      if (*KNOB(KNOB_DEBUG_MISC)) {
        std::cout << "EXEC : " << m_cycle << " : ";
        cur_uop->print();
      }

      tmp_list.push_back(req);
      if (*KNOB(KNOB_UOP_DIRECT_OFFLOAD)) {
        m_simBase->request_done(req);
      } else {
        m_mxp_resp_queue.push_back(req);
      }
    }
  }

  for (auto it = tmp_list.begin(), end = tmp_list.end(); it != end; ++it) {
    m_exec_queue.remove(*it);
  }
}

bool cxlt3_c::push_ramu_req(cxl_req_s* req) {
  bool is_write;
  long addr;

  if (req->m_isuop) {
    auto cur_uop = req->m_uop;
    is_write = cur_uop->is_write();
    addr = static_cast<long>(cur_uop->m_addr);
  } else {
    is_write = req->m_write;
    addr = static_cast<long>(req->m_addr);
  }
  auto req_type = (is_write) ? ramulator::Request::Type::WRITE
                             : ramulator::Request::Type::READ;
  auto cb_func = (is_write) ? m_write_cb_func : m_read_cb_func;

  ramulator::Request ramu_req(addr, req_type, cb_func, req->m_id, req->m_isuop);
  bool accepted = m_ramu_wrapper->send(ramu_req);

  if (accepted) {
    if (req->m_isuop) {
      if (is_write) m_uop_writes[ramu_req.addr].push_back(req);
      else m_uop_reads[ramu_req.addr].push_back(req);
    } else {
      if (is_write) m_mxp_writes[ramu_req.addr].push_back(req);
      else m_mxp_reads[ramu_req.addr].push_back(req);

      ++m_mxp_requestsInFlight;
    }
    req->m_dram_start = m_cycle;
    return true;
  } else {
    return false;
  }
}

// ramulator read callback
void cxlt3_c::readComplete(ramulator::Request &ramu_req) {
  cxl_req_s* req;
  if (ramu_req.is_pim) { // pim req returned
    auto &req_q = m_uop_reads.find(ramu_req.addr)->second;
    req = req_q.front();
    req_q.pop_front();
    if (!req_q.size()) m_uop_reads.erase(ramu_req.addr);

    auto cur_uop = req->m_uop;
    assert(cur_uop);

/* cur_uop->m_done_cycle = m_cycle; */
/* m_exec_queue.push_back(req); */

    free_mshr(cur_uop->m_addr);
    m_cache->insert(cur_uop->m_addr);

    if (*KNOB(KNOB_DEBUG_CALLBACK)) {
      std::cout << "CXL UOP_READ done: " << cur_uop->m_unique_num
        << " lat: " << (m_cycle - req->m_dram_start) << std::endl;
    }
  } else { // normal req returned
    auto &req_q = m_mxp_reads.find(ramu_req.addr)->second;
    req = req_q.front();
    req_q.pop_front();
    if (!req_q.size()) m_mxp_reads.erase(ramu_req.addr);

    --m_mxp_requestsInFlight;
    m_mxp_resp_queue.push_back(req);

    if (*KNOB(KNOB_DEBUG_CALLBACK)) {
      std::cout << "CXL READ done: " << ramu_req.addr 
                << " lat: " << (m_cycle - req->m_dram_start) << std::endl;
    }
  }
}

// ramulator write callback
void cxlt3_c::writeComplete(ramulator::Request &ramu_req) {
  cxl_req_s* req;
  if (ramu_req.is_pim) { // pim req returned
    auto &req_q = m_uop_writes.find(ramu_req.addr)->second;
    req = req_q.front();
    req_q.pop_front();
    if (!req_q.size()) m_uop_writes.erase(ramu_req.addr);

    auto cur_uop = req->m_uop;
    assert(cur_uop);

/* cur_uop->m_done_cycle = m_cycle; */
/* m_exec_queue.push_back(req); */

    free_mshr(cur_uop->m_addr);
    m_cache->insert(cur_uop->m_addr);

    if (*KNOB(KNOB_DEBUG_CALLBACK)) {
      std::cout << "CXL UOP_WRITE done: " << cur_uop->m_unique_num
        << " lat: " << (m_cycle - req->m_dram_start) << std::endl;
    }
  } else { // normal req returned
    auto &req_q = m_mxp_writes.find(ramu_req.addr)->second;
    req = req_q.front();
    req_q.pop_front();
    if (!req_q.size()) m_mxp_writes.erase(ramu_req.addr);

    --m_mxp_requestsInFlight;
    m_mxp_resp_queue.push_back(req);

    if (*KNOB(KNOB_DEBUG_CALLBACK)) {
      std::cout << "CXL WRITE done: " << ramu_req.addr 
                << " lat: " << (m_cycle - req->m_dram_start) << std::endl;
    }
  }
}

void cxlt3_c::free_mshr(Addr addr) {
  auto merged_reqs = m_cache->get_mshr_entries(addr);
  if (merged_reqs != NULL) {
    for (auto merged_req : *merged_reqs) {
      auto merged_uop = merged_req->m_uop;
      merged_uop->m_done_cycle = m_cycle;
      m_exec_queue.push_back(merged_req);
    }
    m_cache->clear_mshr(addr);
  }
}

bool cxlt3_c::check_port(uop_s* uop) {
  auto exec_unit = uop->get_exec_unit();

  if (*KNOB(KNOB_DEBUG_PORT)) {
    m_ports[exec_unit]->print();
  }

  return m_ports[exec_unit]->check_port();
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

  for (auto iter : *m_pending_req) {
    auto addr = iter->m_addr;
    std::cout << "Addr: " << std::hex << addr << ": ";
  }
  std::cout << std::endl;
}

void cxlt3_c::print_cxlt3_uops() {
  std::cout << " ---------- " << m_cycle << " ---------- " << std::endl;
  std::cout << "pend q" << std::endl;
  for (auto req : m_pend_uop_queue) {
    auto uop = req->m_uop;
    uop->print();
  }

  std::cout << "issue q" << std::endl;
  for (auto req : m_issue_queue) {
    auto uop = req->m_uop;
    uop->print();
  }

  std::cout << "exec q" << std::endl;
  for (auto req : m_exec_queue) {
    auto uop = req->m_uop;
    uop->print();
  }
  std::cout << "=========================================" << std::endl;
}

}

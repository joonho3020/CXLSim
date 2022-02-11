/*
Copyright (c) <2012>, <Seoul National University> All rights reserved.

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
 * File         : cache.cc
 * Author       : Joonho
 * Date         : 02/11/2022
 * SVN          : $Id: cache.cc,v 1.5 2021-10-08 21:01:41 kacear Exp $:
 * Description  : NDP cache
 *********************************************************************************************/

#include <iostream>
#include <cassert>
#include <cmath>

#include "cache.h"
#include "packet_info.h"
#include "cxlsim.h"
#include "all_knobs.h"
#include "global_types.h"

namespace cxlsim {

cache_line_s::cache_line_s() {
  init();
}

void cache_line_s::init() {
  m_val = false;
  m_tag = 0;
}

void cache_line_s::print() {
  std::cout << m_val << ":" << std::hex << m_tag << " ";
}

//////////////////////////////////////////////////////////////////////////////

void cache_set_s::init(int assoc) {
  m_assoc = assoc;
  m_valcnt = 0;
  for (int ii = 0; ii < m_assoc; ii++) {
    cache_line_s* new_line = new cache_line_s();
    m_lines.push_back(new_line);
  }
}

bool cache_set_s::lookup(Addr tag, bool update_lru) {
  for (auto it = m_lines.begin(); it != m_lines.end(); it++) {
    auto line = *it;
    if (line->m_val && line->m_tag == tag) {
      if (update_lru) {
        m_lines.erase(it);
        m_lines.push_front(line);
      }
      return true;
    }
  }
  return false;
}

void cache_set_s::insert(Addr tag) {
  cache_line_s* new_line = new cache_line_s();
  new_line->m_val = true;
  new_line->m_tag = tag;

  if (m_valcnt == m_assoc) { // set is full so we need to evict lru
    m_lines.pop_back(); // evict lru
    m_lines.push_front(new_line); // insert mru
  } else { // set is not full, so just insert into a empty line
    for (auto it = m_lines.begin(); it != m_lines.end(); it++) {
      auto line = *it;
      if (!line->m_val) {
        m_lines.erase(it);
        m_lines.push_front(new_line);
        m_valcnt++;
        break;
      }
    }
  }
}

void cache_set_s::print() {
  for (auto line : m_lines) {
    line->print();
  }
  std::cout << std::endl;
}

//////////////////////////////////////////////////////////////////////////////

void mshr_entry_s::init(int capacity) {
  m_valid = true;
  m_capacity = capacity;
  m_reqs.clear();
}

void mshr_entry_s::print(void) {
  for (int ii = 0; ii < (int)m_reqs.size(); ii++) {
    std::cout << m_reqs[ii]->m_uop->m_unique_num << " ";
  }
  std::cout << std::endl;
}

bool mshr_entry_s::insert(cxl_req_s* req) {
  assert(m_valid);
  if (m_capacity >= (int)m_reqs.size()) {
    return false;
  } else {
    m_reqs.push_back(req);
    return true;
  }
}

mshr_s::mshr_s(cxlsim_c* simBase) {
  m_simBase = simBase;
}

void mshr_s::init(int assoc, int capacity) {
  m_assoc = assoc;
  m_capacity = capacity;
}

bool mshr_s::insert(cxl_req_s* req, Addr pfn) {
  if (m_assoc <= (int)m_entries.size()) {
    return false;
  }

  if (m_entries.find(pfn) != m_entries.end()) { // already found entry
    auto entry = m_entries[pfn];
    return entry->insert(req);
  } else { // no entry
    // init new entry
    mshr_entry_s* new_entry = new mshr_entry_s();
    new_entry->init(m_capacity);
    new_entry->m_valid = true;
    new_entry->m_reqs.push_back(req);

    // insert new entry
    m_entries[pfn] = new_entry;
    return true;
  }
}

bool mshr_s::is_first_miss(Addr pfn) {
  if (m_entries.find(pfn) != m_entries.end()) {
    return false;
  } else {
    return true;
  }
}

std::vector<cxl_req_s*>* mshr_s::get_entry(Addr pfn) {
  if (m_entries.find(pfn) == m_entries.end()) { // no entry
    return NULL;
  } else { // found entry
    auto entry = m_entries[pfn];
    assert(entry->m_valid); 
    assert(entry->m_capacity >= (int)entry->m_reqs.size());
    return &(entry->m_reqs);
  }
}

void mshr_s::clear(Addr pfn) {
  assert(m_entries.find(pfn) != m_entries.end());
  m_entries[pfn]->m_reqs.clear();
  m_entries.erase(pfn);
}

void mshr_s::print() {
  for (auto it : m_entries) {
    auto tag = it.first;
    std::cout << std::hex << "pfn: " << tag << std::endl;
    it.second->print();
  }
}

//////////////////////////////////////////////////////////////////////////////

cache_c::cache_c(cxlsim_c* simBase) {
  m_mshr = new mshr_s(m_simBase);
  m_simBase = simBase;
  m_cycle = 0;
}

cache_c::~cache_c() {
  for (auto set : m_sets) {
    for (auto line : set->m_lines) {
      delete line;
    }
    delete set;
  }
  delete m_mshr;
}

void cache_c::init_cache(int set, int assoc, Counter lat) {
  m_set_cnt = set;
  m_lat= lat;
  for (int ii = 0; ii < m_set_cnt; ii++) {
    cache_set_s* new_set = new cache_set_s();
    new_set->init(assoc);
    m_sets.push_back(new_set);
  }

  m_set_bits = (int)log2((float)m_set_cnt);
  m_set_mask = m_set_cnt - 1;
  m_page_offset = *KNOB(KNOB_CACHELINE_OFFSET_BITS);
}

void cache_c::init_mshr(int assoc, int cap) {
/* mshr_s* m_mshr = new mshr_s(m_simBase); */
  m_mshr->init(assoc, cap);
}

bool cache_c::insert_mshr(cxl_req_s* req) {
  return m_mshr->insert(req, get_pfn(req->m_uop->m_addr));
}

std::vector<cxl_req_s*>* cache_c::get_mshr_entries(Addr addr) {
  return m_mshr->get_entry(get_pfn(addr));
}

void cache_c::clear_mshr(Addr addr) {
  m_mshr->clear(get_pfn(addr));
}

bool cache_c::lookup(Addr addr) {
  auto settag = get_settag(addr);
  auto set = m_sets[settag.first];

  return set->lookup(settag.second, true);
}

void cache_c::insert(Addr addr) {
  auto settag = get_settag(addr);
  auto set = m_sets[settag.first];
  set->insert(settag.second);

  // only insert when there is no entry
  assert(!set->lookup(addr, false));
}

Counter cache_c::get_lat() {
  return m_lat;
}

inline Addr cache_c::get_tag(Addr addr) {
  return (addr >> (m_set_bits + m_page_offset));
}

inline int cache_c::get_set(Addr addr) {
  Addr tag_set = (addr >> m_page_offset);
  return (int)(tag_set & m_set_mask);
}

inline Addr cache_c::get_pfn(Addr addr) {
  return (addr >> m_page_offset);
}

std::pair<int, Addr> cache_c::get_settag(Addr addr) {
  return {get_set(addr), get_tag(addr)};
}

bool cache_c::is_first_miss(Addr addr) {
  return m_mshr->is_first_miss(get_pfn(addr));
}

bool cache_c::has_free_mshr() {
  return (m_mshr->m_assoc > (int)m_mshr->m_entries.size());
}

void cache_c::print() {
  std::cout << "========== cache =============" << std::endl;
  for (int ii = 0; ii < m_set_cnt; ii++) {
    std::cout << ii << ": ";
    m_sets[ii]->print();
  }
  std::cout << "========= mshr =================" << std::endl;
  m_mshr->print();
  std::cout << "------------------------------------" << std::endl;
}

} // namespace cxlsim

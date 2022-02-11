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
 * File         : cache.h
 * Author       : Joonho
 * Date         : 02/11/2022
 * SVN          : $Id: cache.h,v 1.5 2021-10-08 21:01:41 kacear Exp $:
 * Description  : NDP cache
 *********************************************************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <list>
#include <vector>

#include "cxlsim.h"
#include "global_defs.h"
#include "global_types.h"

namespace cxlsim {

typedef struct cache_line_s {
  cache_line_s(void);
  void init(void);
  void print(void);

  bool m_val;
  Addr m_tag;
} cache_line_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct cache_set_s {
  void init(int assoc);
  bool lookup(Addr tag, bool update_lru);
  void insert(Addr tag);
  void evict(Addr tag);
  void print(void);

  int m_assoc;
  int m_valcnt;
  std::list<cache_line_s*> m_lines;
} cache_set_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct mshr_entry_s {
  void init(int capacity);
  bool insert(cxl_req_s* req);
  void print(void);

  bool m_valid;
  int m_capacity;
  std::vector<cxl_req_s*> m_reqs;
} mshr_entry_s;

// fully associative mshr
typedef struct mshr_s {
  mshr_s(cxlsim_c* simBase);
  void init(int assoc, int capacity);
  bool insert(cxl_req_s* req, Addr pfn);
  std::vector<cxl_req_s*>* get_entry(Addr pfn);
  void clear(Addr pfn);
  bool is_first_miss(Addr pfn);

  void print(void);

  int m_assoc;
  int m_capacity;
  std::map<Addr, mshr_entry_s*> m_entries;
  cxlsim_c* m_simBase;
} mshr_s;

//////////////////////////////////////////////////////////////////////////////

class cache_c {
public:
  cache_c(cxlsim_c* simBase);
  ~cache_c();
  void init_cache(int set, int assoc, Counter lat);
  void init_mshr(int assoc, int cap);

  bool insert_mshr(cxl_req_s* req);
  std::vector<cxl_req_s*>* get_mshr_entries(Addr addr);
  void clear_mshr(Addr addr);

  bool lookup(Addr addr);
  void insert(Addr addr);
  bool is_first_miss(Addr addr);
  bool has_free_mshr();
  Counter get_lat();

  void print();

private:
  inline Addr get_tag(Addr addr);
  inline int get_set(Addr addr);
  inline Addr get_pfn(Addr addr);
  std::pair<int, Addr> get_settag(Addr addr);

private:
  int m_set_cnt;
  int m_set_bits;
  int m_set_mask;
  int m_page_offset;
  std::vector<cache_set_s*> m_sets;
  Counter m_lat;

  mshr_s* m_mshr;

  Counter m_cycle;
  cxlsim_c* m_simBase;
};

//////////////////////////////////////////////////////////////////////////////

} // namespace CXL

#endif //CACHE_H

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
 * File         : core.h
 * Author       : Joonho
 * Date         : 12/3/2021
 * SVN          : $Id: core.cc 867 2021-12-03 02:28:12Z kacear $:
 * Description  : core for callback test
 *********************************************************************************************/

#ifndef CORE_H
#define CORE_H

#include <list>
#include <map>
#include <string>
#include <fstream>

#include "global_types.h"
#include "global_defs.h"

namespace cxlsim {

/////////////////////////////////////////////////////////////////////////////

// example message struct sent from core to cxl
typedef struct core_req_s {
  core_req_s();
  core_req_s(Addr addr, bool write);
  void init();

  Addr m_addr;
  bool m_write;
} core_req_s;

/////////////////////////////////////////////////////////////////////////////

// simple core
class core_c {
public:
  core_c(cxlsim_c* simBase);
  ~core_c();

  void set_tracefile(std::string filename);
  void insert_request(Addr addr, bool write, Counter cycle);
  void run_a_cycle(bool pll_locked);
  void run_sim();

private:
  void core_callback(Addr addr, bool write, Counter req_id, void *req);

#ifdef CXL_DEBUG
  void check_forward_progress();
#endif

public:
  // for debugging
  Counter m_insert_reqs;
  Counter m_return_reqs;

  // simbase
  cxlsim_c* m_simBase;
  Counter m_cycle;

private:
  std::string m_tracefilename;
  std::list<std::pair<core_req_s*, Counter>> m_pending_q;

#ifdef CXL_DEBUG
  Counter m_in_flight_reqs;
  std::map<Counter, int> m_in_flight_reqs_id;
#endif
};

} //namespace CXL

#endif //CORE_H

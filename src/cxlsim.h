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
 * File         : cxlsim.h
 * Author       : Joonho
 * Date         : 10/10/2021
 * SVN          : $Id: iosim.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : CXL simulation base class
 *********************************************************************************************/

#ifndef CXLSIM_H
#define CXLSIM_H

#include <string>
#include <map>

#include "Callback.h"
#include "global_defs.h"
#include "global_types.h"

namespace CXL {

typedef CallbackBase<void, Addr, bool, void*> callback_t;

typedef enum CLOCK_DOMAIN {
  CLOCK_IO = 0,
  CLOCK_CXLRAM,
  CLOCK_DOMAIN_COUNT
} CLOCK_DOMAIN;

class cxlsim_c {
public:
  /**
   * Constructor
   */
  cxlsim_c();
  
  /**
   * Destructor
   */
  ~cxlsim_c();

  ///////////////////////////////////////////////////////////////////////////
  // API : Outer simulator interface
  ///////////////////////////////////////////////////////////////////////////

  /**
   * Initialize
   */
  void init(int argc, char **argv);

  /** 
   * callback function register
  */
  void register_callback(callback_t* fn);

  /**
   * insert a request to the CXL mem 
   * - it can take a arbitrary pointer type of the outer simulator (void* req)
   *   and return it by the registered callback function
   */
  bool insert_request(Addr addr, bool write, void* req);

  /**
   * Tick a cycle
   */
  void run_a_cycle(bool pll_locked);

  /////////////////////////////////////////////////////////////////////////////

private:
  // initialization
  void init_sim_objects();

  void init_knobs(int argc, char **argv);

  void init_clock_domain();

  /* 
   * called when a request returns to the RC, internally calls the 
   * registered callback function
   */
  void request_done(cxl_req_s* req);

public:
  // simulation objects
  pcie_rc_c* m_rc;
  cxlt3_c* m_cme;

  // external clock
  Counter m_cycle;

  // knobs
  KnobsContainer* m_knobsContainer;
  all_knobs_c* m_knobs;

  // stats
  std::map<std::string, std::ofstream *> m_AllStatsOutputStreams;
  ProcessorStatistics* m_ProcessorStats;
  CoreStatistics* m_coreStatsTemplate;

private:
  // memory pool
  pool_c<cxl_req_s>* m_req_pool;
  pool_c<message_s>* m_msg_pool;
  pool_c<flit_s>* m_flit_pool;

  // callback function for outer simulator
  callback_t* m_trans_done_cb;

  // clock domain
  int m_clock_lcm;
  int *m_domain_freq;
  int *m_domain_count;
  int *m_domain_next;
  int m_clock_internal;
};

}

#endif // CXLSIM_H

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

  void init_knobs(int argc, char **argv);

  /**
   * Initialized
   */
  void init_sim();

  void register_callback(callback_t* fn);

  bool insert_request(Addr addr, bool write, void *req);

  /**
   * Tick a cycle
   */
  void run_a_cycle(bool pll_locked);

  void request_done(cxl_req_s* req);

public:
  pcie_rc_c* m_rc;
  cxlt3_c* m_cme;
  Counter m_cycle;

  // knobs
  KnobsContainer* m_knobsContainer;
  all_knobs_c* m_knobs;

  // stats
  std::map<std::string, std::ofstream *> m_AllStatsOutputStreams;
  ProcessorStatistics* m_ProcessorStats;
  CoreStatistics* m_coreStatsTemplate;

private:
  pool_c<cxl_req_s>* m_req_pool;
  pool_c<message_s>* m_msg_pool;
  pool_c<flit_s>* m_flit_pool;
  callback_t* m_trans_done_cb;
};

}

#endif // CXLSIM_H

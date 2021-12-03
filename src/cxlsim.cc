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
 * SVN          : $Id: cxlsim.cc 867 2009-11-05 02:28:12Z kacear $:
 * Description  : IO domain
 *********************************************************************************************/

#include <iostream>

#include "cxlsim.h"
#include "pcie_rc.h"
#include "cxl_t3.h"
#include "packet_info.h"
#include "utils.h"
#include "all_knobs.h"

namespace CXL {

cxlsim_c::cxlsim_c() {
  // simulation related
  m_cycle = 0;

  // memory pool for packets
  m_msg_pool = new pool_c<message_s>;
  m_flit_pool = new pool_c<flit_s>;
}

cxlsim_c::~cxlsim_c() {
  delete m_rc;
  delete m_cme;
  delete m_msg_pool;
  delete m_flit_pool;
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

void cxlsim_c::init_sim() {
  // io devices
  m_rc = new pcie_rc_c(this);
  m_cme = new cxlt3_c(this);

  m_rc->init(0, true, m_msg_pool, m_flit_pool, m_cme);
  m_cme->init(1, false, m_msg_pool, m_flit_pool, m_rc);
}

// FIXME : add queue here
bool cxlsim_c::push_req(cxl_req_s* req) {
  m_rc->insert_request(req);
  return true;
}

void cxlsim_c::run_a_cycle(bool pll_locked) {
  m_cme->run_a_cycle(pll_locked);
  // FIXME : different clock ratio
  m_cme->run_a_cycle_internal(pll_locked);
  m_rc->run_a_cycle(pll_locked);
  m_cycle++;

  if (m_knobs->KNOB_DEBUG_IO_SYS->getValue()) {
    std::cout << std::endl;
    std::cout << "io cycle : " << std::dec << m_cycle << std::endl;
    // m_simBase->m_dram_controller[0]->print_req();
    m_cme->print_cxlt3_info();
    m_rc->print_rc_info();
  }
}

} // namespace CXL

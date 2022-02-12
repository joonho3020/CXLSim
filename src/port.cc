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
 * File         : port.cc
 * Author       : Joonho
 * Date         : 02/12/2022
 * SVN          : $Id: port.cc,v 1.5 2021-10-08 21:01:41 kacear Exp $:
 * Description  : NDP port
 *********************************************************************************************/

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

#include "port.h"
#include "uop.h"
#include "packet_info.h"
#include "cxlsim.h"
#include "all_knobs.h"
#include "global_types.h"

namespace cxlsim {

port_c::port_c(cxlsim_c* simBase) {
  m_simBase = simBase;
}

void port_c::init(int num_ports, int latency) {
  m_num_ports = num_ports;
  m_latency = latency;
}

bool port_c::check_port() {
  if (m_num_ports > (int)m_using.size()) {
    m_using.push_back(m_latency);
    return true;
  } else {
    return false;
  }
}

struct is_zero {
  bool operator() (const int& value) { return (value == 0); }
};

void port_c::run_a_cycle() {
  for (auto& lat : m_using) {
    lat--;
  }
  m_using.remove_if(is_zero());
}

void port_c::print() {
  for (auto wait_lat : m_using) {
    std::cout << wait_lat << " ";
  }
  std::cout << std::endl;
}

} // namespace cxlsim


#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include "cxlsim.h"
#include "packet_info.h"
#include "global_types.h"

std::vector<CXL::cxl_req_s*> read_trace(CXL::cxlsim_c* simBase) {
  std::vector<CXL::cxl_req_s*> trace;
  std::ifstream file("../trace/sample.txt");

  if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        Addr addr;
        int type;
        std::sscanf(line.c_str(), "%llx %d", &addr, &type);

        CXL::cxl_req_s* new_req = new CXL::cxl_req_s(simBase);
        new_req->m_addr = addr;
        new_req->m_write = (type == 1);
        trace.push_back(new_req);
      }
    file.close();
  }

  return trace;
}

int main(int argc, char **argv) {
  CXL::cxlsim_c* simBase = new CXL::cxlsim_c();
  simBase->init_knobs(argc, argv);
  simBase->init_sim();

  std::list<CXL::cxl_req_s*> pend_q;

  std::ifstream file("../trace/sample.txt");
  if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        Addr addr;
        int type;
        std::sscanf(line.c_str(), "%llx %d", &addr, &type);

        CXL::cxl_req_s* new_req = new CXL::cxl_req_s(simBase);
        new_req->m_addr = addr;
        new_req->m_write = (type == 1);

        simBase->push_req(new_req);
      }
    file.close();
  }

  // FIXME
  for (int ii = 0; ii < 1000; ii++) {
    simBase->run_a_cycle(false);
  }


  return 0;
}

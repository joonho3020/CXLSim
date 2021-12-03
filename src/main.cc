
#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include "cxlsim.h"
#include "core.h"
#include "packet_info.h"
#include "global_types.h"

int main(int argc, char **argv) {
  CXL::cxlsim_c* simBase = new CXL::cxlsim_c();
  simBase->init_knobs(argc, argv);
  simBase->init_sim();

  CXL::core* my_core = new CXL::core(simBase);

  my_core->set_tracefile("../trace/sample.txt");
  my_core->run_sim();

  return 0;
}

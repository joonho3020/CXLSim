#ifndef __CXLRAMULATOR_WRAPPER_H
#define __CXLRAMULATOR_WRAPPER_H

#ifdef RAMULATOR

#include <string>

#include "ramulator/src/Config.h"


namespace ramulator {

class Request;
class MemoryBase;

class CXLRamulatorWrapper
{
private:
  MemoryBase *mem;

public:
  double tCK;
  CXLRamulatorWrapper(const Config &configs, int cacheline, std::string statout);
  ~CXLRamulatorWrapper();
  void tick();
  bool send(Request req);
  void finish(void);
};

} /*namespace ramulator*/

#endif  // CXLRAMULATOR
#endif /*__CXLRAMULATOR_WRAPPER_H*/

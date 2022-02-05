#ifndef __GLOBAL_DEFS_H__
#define __GLOBAL_DEFS_H__

namespace cxlsim {

// class
class cxlsim_c;
class all_knobs_c;
class all_stats_c;

class pcie_ep_c;
class pcie_rc_c;
class cxlt3_c;

class KnobsContainer;
class ProcessorStatistics;
class CoreStatistics;

// templates
template <class T> class pool_c;
template <typename T> class knob_c;

// structs
typedef struct uop_s uop_s;
typedef struct cxl_req_s cxl_req_s;
typedef struct message_s message_s;
typedef struct flit_s flit_s;

} // namespace CXL

#endif //__GLOBAL_DEFS_H__

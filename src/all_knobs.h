#ifndef __ALL_KNOBS_H_INCLUDED__
#define __ALL_KNOBS_H_INCLUDED__

#include "global_types.h"
#include "knob.h"

namespace CXL {

#define KNOB(var) m_simBase->m_knobs->var

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief knob variables holder
///////////////////////////////////////////////////////////////////////////////////////////////
class all_knobs_c {
	public:
		/**
		 * Constructor
		 */
		all_knobs_c();

		/**
		 * Destructor
		 */
		~all_knobs_c();

		/**
		 * Register Knob Variables
		 */
		void registerKnobs(KnobsContainer *container);

	public:
		

	// =========== ../def/io.param.def ===========
		KnobTemplate< float >* KNOB_CLOCK_IO;
		KnobTemplate< bool >* KNOB_CME_ENABLE;
		KnobTemplate< uint64_t >* KNOB_CME_RANGE;
		KnobTemplate< long >* KNOB_CME_LATENCY;
		KnobTemplate< int >* KNOB_PCIE_LANES;
		KnobTemplate< float >* KNOB_PCIE_PER_LANE_BW;
		KnobTemplate< int >* KNOB_PCIE_VC_CNT;
		KnobTemplate< int >* KNOB_PCIE_TXVC_CAPACITY;
		KnobTemplate< int >* KNOB_PCIE_RXVC_CAPACITY;
		KnobTemplate< int >* KNOB_PCIE_TXDLL_CAPACITY;
		KnobTemplate< int >* KNOB_PCIE_PHYS_CAPACITY;
		KnobTemplate< int >* KNOB_PCIE_TXREPLAY_CAPACITY;
		KnobTemplate< uint64_t >* KNOB_PCIE_TXTRANS_LATENCY;
		KnobTemplate< uint64_t >* KNOB_PCIE_TXDLL_LATENCY;
		KnobTemplate< uint64_t >* KNOB_PCIE_RXTRANS_LATENCY;
		KnobTemplate< uint64_t >* KNOB_PCIE_RXDLL_LATENCY;
		KnobTemplate< uint64_t >* KNOB_PCIE_ARBMUX_LATENCY;
		KnobTemplate< int >* KNOB_PCIE_MAX_MSG_PER_FLIT;
		KnobTemplate< int >* KNOB_PCIE_FLIT_BITS;
		KnobTemplate< int >* KNOB_PCIE_DATA_SLOTS_PER_FLIT;
		KnobTemplate< int >* KNOB_PCIE_DATA_MSG_BITS;
		KnobTemplate< int >* KNOB_PCIE_REQ_MSG_BITS;
		KnobTemplate< int >* KNOB_PCIE_RWD_MSG_BITS;
		KnobTemplate< int >* KNOB_PCIE_NDR_MSG_BITS;
		KnobTemplate< int >* KNOB_PCIE_DRS_MSG_BITS;
		KnobTemplate< std::string >* KNOB_STATISTICS_OUT_DIRECTORY;
		KnobTemplate< int >* KNOB_DEBUG_IO_SYS;
		KnobTemplate< std::string >* KNOB_RAMULATOR_CONFIG_FILE;
		KnobTemplate< int >* KNOB_RAMULATOR_CACHELINE_SIZE;
		KnobTemplate< int >* KNOB_NUM_SIM_CORES;

};

} // namespace CXL

#endif //__ALL_KNOBS_H_INCLUDED__

#include "all_knobs.h"

#include <string>

namespace CXL {

all_knobs_c::all_knobs_c() {
	

	// =========== ../def/io.param.def ===========
	KNOB_CLOCK_IO = new KnobTemplate< float > ("clock_io", 0.8);
	KNOB_CME_ENABLE = new KnobTemplate< bool > ("cme_enable", false);
	KNOB_CME_RANGE = new KnobTemplate< uint64_t > ("cme_addr_min", 0);
	KNOB_CME_LATENCY = new KnobTemplate< long > ("cme_latency", 300);
	KNOB_PCIE_LANES = new KnobTemplate< int > ("pcie_lanes", 8);
	KNOB_PCIE_PER_LANE_BW = new KnobTemplate< float > ("pcie_per_lane_bw", 32);
	KNOB_PCIE_VC_CNT = new KnobTemplate< int > ("pcie_vc_cnt", 2);
	KNOB_PCIE_TXVC_CAPACITY = new KnobTemplate< int > ("pcie_txvc_capacity", 8);
	KNOB_PCIE_RXVC_CAPACITY = new KnobTemplate< int > ("pcie_rxvc_capacity", 8);
	KNOB_PCIE_TXDLL_CAPACITY = new KnobTemplate< int > ("pcie_txdll_capacity", 8);
	KNOB_PCIE_PHYS_CAPACITY = new KnobTemplate< int > ("pcie_phys_capacity", 8);
	KNOB_PCIE_TXREPLAY_CAPACITY = new KnobTemplate< int > ("pcie_txreplay_capacity", 8);
	KNOB_PCIE_TXTRANS_LATENCY = new KnobTemplate< uint64_t > ("pcie_txtrans_latency", 5);
	KNOB_PCIE_TXDLL_LATENCY = new KnobTemplate< uint64_t > ("pcie_txdll_latency", 5);
	KNOB_PCIE_RXTRANS_LATENCY = new KnobTemplate< uint64_t > ("pcie_rxtrans_latency", 5);
	KNOB_PCIE_RXDLL_LATENCY = new KnobTemplate< uint64_t > ("pcie_rxdll_latency", 5);
	KNOB_PCIE_ARBMUX_LATENCY = new KnobTemplate< uint64_t > ("pcie_arbmux_latency", 2);
	KNOB_PCIE_MAX_MSG_PER_FLIT = new KnobTemplate< int > ("pcie_max_msg_per_flit", 4);
	KNOB_PCIE_FLIT_BITS = new KnobTemplate< int > ("pcie_flit_bits", 544);
	KNOB_PCIE_DATA_SLOTS_PER_FLIT = new KnobTemplate< int > ("pcie_data_slots_per_flit", 4);
	KNOB_PCIE_DATA_MSG_BITS = new KnobTemplate< int > ("pcie_data_msg_bits", 128);
	KNOB_PCIE_REQ_MSG_BITS = new KnobTemplate< int > ("pcie_req_msg_bits", 87);
	KNOB_PCIE_RWD_MSG_BITS = new KnobTemplate< int > ("pcie_rwd_msg_bits", 87);
	KNOB_PCIE_NDR_MSG_BITS = new KnobTemplate< int > ("pcie_ndr_msg_bits", 30);
	KNOB_PCIE_DRS_MSG_BITS = new KnobTemplate< int > ("pcie_drs_msg_bits", 40);
	KNOB_STATISTICS_OUT_DIRECTORY = new KnobTemplate< std::string > ("out", ".");
	KNOB_DEBUG_IO_SYS = new KnobTemplate< int > ("debug_io_sys", 0);
	KNOB_RAMULATOR_CONFIG_FILE = new KnobTemplate< std::string > ("ramulator_config_file", "DDR4-config.cfg");
	KNOB_RAMULATOR_CACHELINE_SIZE = new KnobTemplate< int > ("ramulator_cacheline_size", 64);
	KNOB_NUM_SIM_CORES = new KnobTemplate< int > ("num_sim_cores", 1);
}

all_knobs_c::~all_knobs_c() {
	delete KNOB_CLOCK_IO;
	delete KNOB_CME_ENABLE;
	delete KNOB_CME_RANGE;
	delete KNOB_CME_LATENCY;
	delete KNOB_PCIE_LANES;
	delete KNOB_PCIE_PER_LANE_BW;
	delete KNOB_PCIE_VC_CNT;
	delete KNOB_PCIE_TXVC_CAPACITY;
	delete KNOB_PCIE_RXVC_CAPACITY;
	delete KNOB_PCIE_TXDLL_CAPACITY;
	delete KNOB_PCIE_PHYS_CAPACITY;
	delete KNOB_PCIE_TXREPLAY_CAPACITY;
	delete KNOB_PCIE_TXTRANS_LATENCY;
	delete KNOB_PCIE_TXDLL_LATENCY;
	delete KNOB_PCIE_RXTRANS_LATENCY;
	delete KNOB_PCIE_RXDLL_LATENCY;
	delete KNOB_PCIE_ARBMUX_LATENCY;
	delete KNOB_PCIE_MAX_MSG_PER_FLIT;
	delete KNOB_PCIE_FLIT_BITS;
	delete KNOB_PCIE_DATA_SLOTS_PER_FLIT;
	delete KNOB_PCIE_DATA_MSG_BITS;
	delete KNOB_PCIE_REQ_MSG_BITS;
	delete KNOB_PCIE_RWD_MSG_BITS;
	delete KNOB_PCIE_NDR_MSG_BITS;
	delete KNOB_PCIE_DRS_MSG_BITS;
	delete KNOB_STATISTICS_OUT_DIRECTORY;
	delete KNOB_DEBUG_IO_SYS;
	delete KNOB_RAMULATOR_CONFIG_FILE;
	delete KNOB_RAMULATOR_CACHELINE_SIZE;
	delete KNOB_NUM_SIM_CORES;
}

void all_knobs_c::registerKnobs(KnobsContainer *container) {
	

	// =========== ../def/io.param.def ===========
	container->insertKnob( KNOB_CLOCK_IO );
	container->insertKnob( KNOB_CME_ENABLE );
	container->insertKnob( KNOB_CME_RANGE );
	container->insertKnob( KNOB_CME_LATENCY );
	container->insertKnob( KNOB_PCIE_LANES );
	container->insertKnob( KNOB_PCIE_PER_LANE_BW );
	container->insertKnob( KNOB_PCIE_VC_CNT );
	container->insertKnob( KNOB_PCIE_TXVC_CAPACITY );
	container->insertKnob( KNOB_PCIE_RXVC_CAPACITY );
	container->insertKnob( KNOB_PCIE_TXDLL_CAPACITY );
	container->insertKnob( KNOB_PCIE_PHYS_CAPACITY );
	container->insertKnob( KNOB_PCIE_TXREPLAY_CAPACITY );
	container->insertKnob( KNOB_PCIE_TXTRANS_LATENCY );
	container->insertKnob( KNOB_PCIE_TXDLL_LATENCY );
	container->insertKnob( KNOB_PCIE_RXTRANS_LATENCY );
	container->insertKnob( KNOB_PCIE_RXDLL_LATENCY );
	container->insertKnob( KNOB_PCIE_ARBMUX_LATENCY );
	container->insertKnob( KNOB_PCIE_MAX_MSG_PER_FLIT );
	container->insertKnob( KNOB_PCIE_FLIT_BITS );
	container->insertKnob( KNOB_PCIE_DATA_SLOTS_PER_FLIT );
	container->insertKnob( KNOB_PCIE_DATA_MSG_BITS );
	container->insertKnob( KNOB_PCIE_REQ_MSG_BITS );
	container->insertKnob( KNOB_PCIE_RWD_MSG_BITS );
	container->insertKnob( KNOB_PCIE_NDR_MSG_BITS );
	container->insertKnob( KNOB_PCIE_DRS_MSG_BITS );
	container->insertKnob( KNOB_STATISTICS_OUT_DIRECTORY );
	container->insertKnob( KNOB_DEBUG_IO_SYS );
	container->insertKnob( KNOB_RAMULATOR_CONFIG_FILE );
	container->insertKnob( KNOB_RAMULATOR_CACHELINE_SIZE );
	container->insertKnob( KNOB_NUM_SIM_CORES );
}

} // namespace CXL 


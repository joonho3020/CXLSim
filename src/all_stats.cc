#include "all_stats.h"
#include "statsEnums.h"

namespace CXL {

all_stats_c::all_stats_c(ProcessorStatistics* procStat) {
	
	// ============= ../def/io.stat.def =============
	m_PCIE_GOODPUT_BASE = new COUNT_Stat("PCIE_GOODPUT_BASE", "io.stat.out", PCIE_GOODPUT_BASE);
	m_AVG_PCIE_GOODPUT = new RATIO_Stat("AVG_PCIE_GOODPUT",  "io.stat.out", AVG_PCIE_GOODPUT, PCIE_GOODPUT_BASE, procStat);
	m_PCIE_FLIT_BASE = new COUNT_Stat("PCIE_FLIT_BASE", "io.stat.out", PCIE_FLIT_BASE);
	m_AVG_PCIE_PHYS_LATENCY = new RATIO_Stat("AVG_PCIE_PHYS_LATENCY",  "io.stat.out", AVG_PCIE_PHYS_LATENCY, PCIE_FLIT_BASE, procStat);
	m_PCIE_TXTRANS_BASE = new COUNT_Stat("PCIE_TXTRANS_BASE", "io.stat.out", PCIE_TXTRANS_BASE);
	m_AVG_PCIE_TXTRANS_LATENCY = new RATIO_Stat("AVG_PCIE_TXTRANS_LATENCY",  "io.stat.out", AVG_PCIE_TXTRANS_LATENCY, PCIE_TXTRANS_BASE, procStat);
	m_PCIE_TXDLL_BASE = new COUNT_Stat("PCIE_TXDLL_BASE", "io.stat.out", PCIE_TXDLL_BASE);
	m_AVG_PCIE_TXDLL_LATENCY = new RATIO_Stat("AVG_PCIE_TXDLL_LATENCY",  "io.stat.out", AVG_PCIE_TXDLL_LATENCY, PCIE_TXDLL_BASE, procStat);
	m_PCIE_RXDLL_BASE = new COUNT_Stat("PCIE_RXDLL_BASE", "io.stat.out", PCIE_RXDLL_BASE);
	m_AVG_PCIE_RXDLL_LATENCY = new RATIO_Stat("AVG_PCIE_RXDLL_LATENCY",  "io.stat.out", AVG_PCIE_RXDLL_LATENCY, PCIE_RXDLL_BASE, procStat);
	m_PCIE_RXTRANS_BASE = new COUNT_Stat("PCIE_RXTRANS_BASE", "io.stat.out", PCIE_RXTRANS_BASE);
	m_AVG_PCIE_RXTRANS_LATENCY = new RATIO_Stat("AVG_PCIE_RXTRANS_LATENCY",  "io.stat.out", AVG_PCIE_RXTRANS_LATENCY, PCIE_RXTRANS_BASE, procStat);
}

all_stats_c::~all_stats_c() {
	delete m_PCIE_GOODPUT_BASE;
	delete m_AVG_PCIE_GOODPUT;
	delete m_PCIE_FLIT_BASE;
	delete m_AVG_PCIE_PHYS_LATENCY;
	delete m_PCIE_TXTRANS_BASE;
	delete m_AVG_PCIE_TXTRANS_LATENCY;
	delete m_PCIE_TXDLL_BASE;
	delete m_AVG_PCIE_TXDLL_LATENCY;
	delete m_PCIE_RXDLL_BASE;
	delete m_AVG_PCIE_RXDLL_LATENCY;
	delete m_PCIE_RXTRANS_BASE;
	delete m_AVG_PCIE_RXTRANS_LATENCY;
}

void all_stats_c::initialize(ProcessorStatistics* m_ProcessorStats, CoreStatistics* m_coreStatsTemplate) {
	m_ProcessorStats->globalStats()->addStatistic(m_PCIE_GOODPUT_BASE);
	m_ProcessorStats->globalStats()->addStatistic(m_AVG_PCIE_GOODPUT);
	m_ProcessorStats->globalStats()->addStatistic(m_PCIE_FLIT_BASE);
	m_ProcessorStats->globalStats()->addStatistic(m_AVG_PCIE_PHYS_LATENCY);
	m_ProcessorStats->globalStats()->addStatistic(m_PCIE_TXTRANS_BASE);
	m_ProcessorStats->globalStats()->addStatistic(m_AVG_PCIE_TXTRANS_LATENCY);
	m_ProcessorStats->globalStats()->addStatistic(m_PCIE_TXDLL_BASE);
	m_ProcessorStats->globalStats()->addStatistic(m_AVG_PCIE_TXDLL_LATENCY);
	m_ProcessorStats->globalStats()->addStatistic(m_PCIE_RXDLL_BASE);
	m_ProcessorStats->globalStats()->addStatistic(m_AVG_PCIE_RXDLL_LATENCY);
	m_ProcessorStats->globalStats()->addStatistic(m_PCIE_RXTRANS_BASE);
	m_ProcessorStats->globalStats()->addStatistic(m_AVG_PCIE_RXTRANS_LATENCY);










}

} // namespace CXL

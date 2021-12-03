#ifndef _ALL_STATS_C_INCLUDED_
#define _ALL_STATS_C_INCLUDED_
#include "statistics.h"

namespace CXL {

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief knob variables holder
///////////////////////////////////////////////////////////////////////////////////////////////
class all_stats_c {
	public:
		/**
		 * Constructor
		 */
		all_stats_c(ProcessorStatistics* procStat);

		/**
		 * Constructor
		 */
		~all_stats_c();

		/**
		 * Constructor
		 */
		void initialize(ProcessorStatistics*, CoreStatistics*);

		
		// ============= ../def/io.stat.def =============
		COUNT_Stat* m_PCIE_GOODPUT_BASE;
		RATIO_Stat* m_AVG_PCIE_GOODPUT;
		COUNT_Stat* m_PCIE_FLIT_BASE;
		RATIO_Stat* m_AVG_PCIE_PHYS_LATENCY;
		COUNT_Stat* m_PCIE_TXTRANS_BASE;
		RATIO_Stat* m_AVG_PCIE_TXTRANS_LATENCY;
		COUNT_Stat* m_PCIE_TXDLL_BASE;
		RATIO_Stat* m_AVG_PCIE_TXDLL_LATENCY;
		COUNT_Stat* m_PCIE_RXDLL_BASE;
		RATIO_Stat* m_AVG_PCIE_RXDLL_LATENCY;
		COUNT_Stat* m_PCIE_RXTRANS_BASE;
		RATIO_Stat* m_AVG_PCIE_RXTRANS_LATENCY;
};

} // namespace CXL

#endif //ALL_STATS_C_INCLUDED

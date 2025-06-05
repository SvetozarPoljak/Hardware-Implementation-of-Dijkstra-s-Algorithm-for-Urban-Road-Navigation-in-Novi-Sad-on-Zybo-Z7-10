#include <systemc>
#include "tb_vp.hpp"
#include "vp.hpp"

using namespace sc_core;

int sc_main(int argc, char* argv[])
{
	//Instanciranje modula i signala kojima ce biti povezani
	vp vp_platform("vp");
	tb_vp testbench("tb");
	sc_core::sc_signal<sc_dt::sc_uint<2>> interrupt_signal;
	
	//Povezivanje modula
	testbench.isoc.bind(vp_platform.soc_cpu);
	vp_platform.interrupt.bind(interrupt_signal);
	testbench.IRQ_F2P.bind(interrupt_signal);
	
	//Postavljanje tlm::tlm_global_quantum
	tlm::tlm_global_quantum::instance().set(sc_time(10, SC_NS));
	
	//Pocetak simulacije
	sc_start();
	
	std::cout << "\nSimulation ended at: " << sc_time_stamp() << std::endl;
	
	return 0;
}

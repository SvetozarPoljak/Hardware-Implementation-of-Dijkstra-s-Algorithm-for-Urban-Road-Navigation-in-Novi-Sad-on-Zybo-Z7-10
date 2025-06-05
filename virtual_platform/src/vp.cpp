#include "vp.hpp"
#include <iostream>

using namespace sc_core;

SC_HAS_PROCESS(vp);

//Konstruktor objekta klase vp. U njemu se povezuju svi moduli koji pripadaju virtuelnoj platformi
vp::vp(sc_module_name name):
	sc_module(name),
	ic_module("ic_module"),
	ip_module("ip_module"),
	bram_module("bram_module"),
	dma_module("dma_module"),
	ram_module("ddr3_ram_module")
{
	SC_METHOD(concat);
	dont_initialize();
	sensitive << dma_interrupt_signal << ip_interrupt_signal;
	
	soc_cpu.register_b_transport(this, &vp::b_transport_cpu);
	
	s_bus.bind(ic_module.soc_cpu);
	
	dma_module.isoc_ic.bind(ic_module.soc_dma);
	dma_module.dma_interrupt.bind(dma_interrupt_signal);
	
	ic_module.isoc_ip.bind(ip_module.soc_ic);
	ic_module.isoc_bram.bind(bram_module.soc_ic);
	ic_module.isoc_dma.bind(dma_module.soc_ic);
	ic_module.isoc_ram.bind(ram_module.soc_ic);
	
	ip_module.isoc_bram.bind(bram_module.soc_ip);
	ip_module.isoc_dma.bind(dma_module.soc_ip);
	ip_module.ip_interrupt.bind(ip_interrupt_signal);
	
	SC_REPORT_INFO("VP", "Platform is constructed");
}

void vp::b_transport_cpu(pl_t& pl, sc_time& delay)
{
	s_bus->b_transport(pl, delay);
	//SC_REPORT_INFO("VP", "Transaction passes...");
}

void vp::concat()
{
	sc_uint<2> intr;

	intr[1] = dma_interrupt_signal.read();
	intr[0] = ip_interrupt_signal.read(); 
	
	SC_REPORT_INFO("VP", "prekid registrovan!");
	
	cout << endl << intr[1] << intr[0] << endl;
	
	interrupt.write(intr);
}

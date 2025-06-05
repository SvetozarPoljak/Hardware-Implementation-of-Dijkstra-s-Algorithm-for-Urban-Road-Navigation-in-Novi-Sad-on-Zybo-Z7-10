#ifndef _VP_HPP_
#define _VP_HPP_

#include <systemc>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include "ic.hpp"
#include "ip.hpp"
#include "bram_mem.hpp"
#include "dma.hpp"
#include "ddr3.hpp"

class vp:
	sc_core::sc_module
{
	public:
		vp(sc_core::sc_module_name);

		/*
			Klasa koja modeluje virtuelnu platformu.
			Sastoji se iz IP, BRAM, DMA, RAM i INTERCONNECT modula.
			Njen interfejs sadrzi:
				- Signal: 
					1. interrupt - izlazni signal 
				- TLM jednostavan prikljucak soc_cpu - prikljucak ka CPU modulu
		*/	
		tlm_utils::simple_target_socket<vp> soc_cpu;
		sc_core::sc_out<sc_dt::sc_uint<2>> interrupt;
	protected:
		sc_core::sc_signal<bool> ip_interrupt_signal;
		sc_core::sc_signal<bool> dma_interrupt_signal;
		
		tlm_utils::simple_initiator_socket<vp> s_bus;
		ddr3_ram ram_module;
		interconnect ic_module;
		ip ip_module;
		bram_mem bram_module;
		dma dma_module;
		
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
		void b_transport_cpu(pl_t&, sc_core::sc_time&);
		
		void concat();
};

#endif

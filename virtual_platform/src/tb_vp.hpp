#ifndef _TB_VP_HPP_
#define _TB_VP_HPP_

#include "httplib.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <math.h>

#include <fstream>
#include <cstdio>
#include <sstream>

#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <tlm_utils/simple_initiator_socket.h>

class tb_vp:
	public sc_core::sc_module
{
	public:
		tb_vp(sc_core::sc_module_name);
		
		/*
			Testbenc modul implementira dva CPU jezgra i GIC kontroler. 
			Njegov interfejs prema virtuelnoj platformi podrazumeva: 
			
			- signal, IRQ_F2P
			- TLM jednostavan prikljucak, isoc 	
		*/
		tlm_utils::simple_initiator_socket<tb_vp> isoc;
		sc_core::sc_in<sc_dt::sc_uint<2>> IRQ_F2P;
	
	protected:
		httplib::Server svr;
	
		sc_core::sc_event ip_interrupt_event;
		sc_core::sc_event dma_interrupt_event;
		sc_core::sc_event relaxations_are_done;
		sc_core::sc_event end_of_simulation;
		
		//sinhronizacija izmedju core0 i core1:
		sc_core::sc_event new_locations_uploaded;
		sc_core::sc_event new_output_is_ready;
		
		void core0();
		void core1();
		void gic();
		void exit_simulation();
		
		void IP_ISR();
		void DMA_ISR();
		
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
		
		//ofset na baznu adresu u koju se u ramu upisuju izvrsene relaksacije
		int index;
		
		uint8_t ip_register_content;
		uint32_t dma_register_content;
};

#endif

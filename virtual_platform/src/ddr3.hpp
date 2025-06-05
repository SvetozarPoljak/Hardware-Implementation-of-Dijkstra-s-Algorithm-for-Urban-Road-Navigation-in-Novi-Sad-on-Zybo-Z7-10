#ifndef _DDR3_RAM_MEM_HPP_
#define _DDR3_RAM_MEM_HPP_

#include <systemc>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <vector>
#include "ddr3_addr.hpp"
#include "graph_dimensions.hpp"

using namespace std;

class ddr3_ram:
	public sc_core::sc_module
{
	public:
		ddr3_ram(sc_core::sc_module_name);
		
		/*
			ddr3_ram modul. 
			Njegov interfejs sadrzi: 
			-jednostavan TLM prikljucak:
				1. soc_ic - prikljucak ka IC modulu  	
		*/		
		tlm_utils::simple_target_socket<ddr3_ram> soc_ic;
		
		//U ram memoriji se nalaze: 
		//	1. Koordinate temena mreze saobracajnica Novog Sada
		float graph_latitudes[VERTEX_NUM]; 		// adrese: 0 - 9121
		float graph_longitudes[VERTEX_NUM];		// adrese: 9122 - 18243
		
		//	2. relaxation history
		std::vector<uint32_t> relaxation_history; 	// adrese: 18244 - (relaxation_history.size()-1) + 18244
		//Ucitavanje podataka mape u ram memoriju, smatrace se da akcija ne trosi simulaciono vreme
		void load_data_for_ddr3();
		
	protected:
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
		void b_transport_ic(pl_t&, sc_core::sc_time&);
};

#endif

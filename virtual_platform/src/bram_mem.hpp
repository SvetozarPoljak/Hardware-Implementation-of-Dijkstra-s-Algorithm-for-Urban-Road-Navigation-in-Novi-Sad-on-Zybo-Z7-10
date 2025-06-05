#ifndef _BRAM_MEM_HPP_
#define _BRAM_MEM_HPP_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include "bram_dma_ip_consts.hpp"
#include "graph_dimensions.hpp"

class bram_mem:
	public sc_core::sc_module
{
	public:
		bram_mem(sc_core::sc_module_name);
		
		/*
			BRAM modul. 
			Njegov interfejs sadrzi: 
			- dva jednostavna TLM prikljucka:
				1. soc_ip - prikljucak ka IP modulu  
				2. soc_ic - ka INTERCONNECT modulu 	
		*/		
		tlm_utils::simple_target_socket<bram_mem> soc_ip;
		tlm_utils::simple_target_socket<bram_mem> soc_ic;
		
	protected:
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
		void b_transport_ip(pl_t&, sc_core::sc_time&);
		void b_transport_ic(pl_t&, sc_core::sc_time&);
		
		//Ucitavanje podataka mape u bram memoriju, smatrace se da akcija ne trosi simulaciono vreme
		void load_data_for_bram();
		
		//Broj 32-bitnih lokacija u BRAM-u, s obzirom da BRAM ima 270kB brze memorije
		static const unsigned BRAM_SIZE = 76800;
		unsigned bram[BRAM_SIZE];	
		
		//U BRAM memoriji ce se nalaziti i Read-Only podaci kojima IP modul cesto pristupa:
		unsigned head[ARC_NUM];		   		
		unsigned tail[ARC_NUM];		   		
		unsigned edge_weights[ARC_NUM];	   			
};

#endif

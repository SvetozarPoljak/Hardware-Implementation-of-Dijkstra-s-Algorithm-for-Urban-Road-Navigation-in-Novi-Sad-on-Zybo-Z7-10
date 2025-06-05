#ifndef _IP_HPP_
#define _IP_HPP_

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include "bram_dma_ip_consts.hpp"
#include "graph_dimensions.hpp"
#include "ddr3_addr.hpp"

class ip:
	public sc_core::sc_module
{
	public:
		ip(sc_core::sc_module_name);
		
		/*
			IP modul implementira funkciju relaxationProcess iz izvrsne specifikacije. 
			Njegov interfejs prema virtuelnoj platformi podrazumeva: 
			
			- signal: 
				1. ip_interrupt - izlazni signal signal koji javlja da je izvrsen proces relaksacije ili da je potrebno isprazniti pomocni bafer
			- tri jednostavna TLM prikljucka:
				1. soc_ic   - prikljucak ka interconnect modulu  
				2. isoc_bram - prikljucak ka BRAM modulu
				3. isoc_dma  - prikljucak ka DMA modulu  	
		*/
		tlm_utils::simple_target_socket<ip> soc_ic;
		tlm_utils::simple_initiator_socket<ip> isoc_bram;
		tlm_utils::simple_initiator_socket<ip> isoc_dma;
		
		sc_core::sc_out<bool> ip_interrupt;
	protected:
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
        	
        	sc_core::sc_event register_state_updated; 
        	
        	//stanje IP modula
        	typedef enum {RESET, DMA_STREAM, DO_RELAXATION, END, IDLE} ip_state_t;
        	ip_state_t state;
        	
        	//pomocne promenljive za proces relaksacije temena grafa / interni registri koji softver ne vidi 
        	//{
        	node_id_t temp_id, temp_head, temp_tail;
        	weight_t temp_edge_weight;
        	cost_t temp_cost_from_start_vertex;
        	
        	cost_t min_cost;
		node_id_t id_of_min_cost_vertex;
		bool there_are_unvisited_vertices_in_table;
		bool all_costs_are_inf;
		bool next_iteration;
		bool temp_vertex_is_visited;
     
        	//brojac koji ce se koristiti
		int i;
        	//}
        	
        	//Pomocni bafer
        	uint32_t relaxation_buffer[50];
        	int index;
        	
        	/*	SAHE element, ip_cfg_reg koji na svom: 
        		0. bitu: signalizira da ip radi reset, IP_RESET_BIT
        		1. bitu: signalizira da ip radi radi relaksacije, IP_DO_RELAXATION_BIT
        		2. bitu: signalizira da ip zahteva od procesora konfiguraciju dma modula kako bi se podaci iz pomocnog bafera upisali u dma modul, IP_DMA_REQ_BIT
        		3. bitu: signalizira da ip javlja cpu da proces strimovanja podataka traje, IP_DMA_STREAM_BIT
        		4. bitu: signalizira da ip javlja cpu da je proces relaksacija zavrsen za dati upit, IP_END_BIT  		
        	
        	*/
        	uint8_t ip_cfg_reg;
        	
		void b_transport_ic(pl_t&, sc_core::sc_time&);
		
		void relaxation();
};

#endif

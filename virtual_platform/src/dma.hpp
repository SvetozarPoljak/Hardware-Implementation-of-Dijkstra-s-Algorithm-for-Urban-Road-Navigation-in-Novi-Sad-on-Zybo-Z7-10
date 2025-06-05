#ifndef _DMA_HPP_
#define _DMA_HPP_

#include "bram_dma_ip_consts.hpp"
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>

class dma:
	public sc_core::sc_module
{
	public:
		dma(sc_core::sc_module_name);

		/*
			DMA modul obezbedjuje direktan pristup memoriji, bez posredstva procesora.
			Njegov interfejs sadrzi:
			- signal: 
				1. dma_interrupt - izlazni signal signal koji javlja da je izvrsen proces prenosa podataka u ram
			- tlm 2.0 prikljucke:	
				1. isoc_ic - prikljucak koji salje tlm paket interconnectu, RAM memoriji
				2. soc_ip  - prikljucak koji prima tlm paket iz IP modula
				3. soc_ic  - prikljucak koji prima tlm paket od cpu modula
			
		*/
		sc_core::sc_out<bool> dma_interrupt;
		tlm_utils::simple_target_socket<dma> soc_ip;
		tlm_utils::simple_initiator_socket<dma> isoc_ic;
		tlm_utils::simple_target_socket<dma> soc_ic;
		
	protected:
		sc_core::sc_event register_state_updated;
		
		//stanje DMA modula
        	typedef enum {RESET, DMA_STREAM, END, IDLE} dma_state_t;
        	dma_state_t state;
		
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
		void b_transport_ic(pl_t&, sc_core::sc_time&);
		void b_transport_ip(pl_t&, sc_core::sc_time&);
		
		//DMA SAHE
		uint32_t S2MM_DMACR;
		uint32_t S2MM_DA;
		uint32_t S2MM_LENGTH;
		
		int transaction_number;
		
		void dma_method();
};

#endif

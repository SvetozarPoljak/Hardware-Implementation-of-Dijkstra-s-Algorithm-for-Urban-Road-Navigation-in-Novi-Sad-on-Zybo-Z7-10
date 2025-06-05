#ifndef _IC_HPP_
#define _IC_HPP_

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

class interconnect:
	public sc_core::sc_module
{
	public:
		interconnect(sc_core::sc_module_name);

		/*
			Interconnect modul memorijski mapira RAM, DMA, IP i BRAM modul.
			Njegov interfejs sadrzi jednostavne tlm prikljucke:
		*/
		tlm_utils::simple_target_socket<interconnect> soc_cpu;
		tlm_utils::simple_initiator_socket<interconnect> isoc_bram;
		tlm_utils::simple_initiator_socket<interconnect> isoc_ip;
		tlm_utils::simple_initiator_socket<interconnect> isoc_dma;
		tlm_utils::simple_target_socket<interconnect> soc_dma;
		tlm_utils::simple_initiator_socket<interconnect> isoc_ram;
		
	protected:
		typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
		void b_transport_cpu(pl_t&, sc_core::sc_time&);
		void b_transport_dma(pl_t&, sc_core::sc_time&);
};

#endif

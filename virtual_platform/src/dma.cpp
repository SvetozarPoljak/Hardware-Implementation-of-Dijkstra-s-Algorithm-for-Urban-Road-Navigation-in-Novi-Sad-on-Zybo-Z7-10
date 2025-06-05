#include "dma.hpp"
#include <sstream>
#include <string>

using namespace std;
using namespace tlm;
using namespace sc_core;
using namespace sc_dt;

SC_HAS_PROCESS(dma);

//Konstruktor objekta klase dma
dma::dma(sc_module_name name):
	sc_module(name),
	soc_ic("dma_soc_ic"),
	isoc_ic("dma_isoc_ic"),
	soc_ip("dma_soc_ip"),
	dma_interrupt("dma_interrupt")
{
	soc_ic.register_b_transport(this, &dma::b_transport_ic);
	soc_ip.register_b_transport(this, &dma::b_transport_ip);
	SC_METHOD(dma_method);
	dont_initialize();
	sensitive << register_state_updated;
	
	state = IDLE;	
	transaction_number = 0;
}

//Interpretacija tlm komunikacije prema interconnect modulu
void dma::b_transport_ic(pl_t& pl, sc_time& offset)
{
	tlm_command cmd     = pl.get_command();
	uint64 addr	    = pl.get_address();
	unsigned char* data = pl.get_data_ptr();
	uint32_t len	    = pl.get_data_length();
	stringstream ss;	
	switch(cmd)
	{
		
		case TLM_WRITE_COMMAND:
		{
			switch(addr)
			{
				case DMA_S2MM_DMACR:
					std::memcpy(&S2MM_DMACR, data, len);
					if(S2MM_DMACR & 0x00000004) // reset
					{
						state = RESET;
						register_state_updated.notify();
					}
					else if(S2MM_DMACR & 0x00000002)
					{
						state = DMA_STREAM;
						register_state_updated.notify();
					}
					else 
					{
						state = IDLE;
						register_state_updated.notify();
					}
					pl.set_response_status(TLM_OK_RESPONSE);
					break;
				case DMA_S2MM_DA:
					std::memcpy(&S2MM_DA, data, len);
					pl.set_response_status(TLM_OK_RESPONSE);
					break;
				case DMA_S2MM_LENGTH:
					std::memcpy(&S2MM_LENGTH, data, len); 
					pl.set_response_status(TLM_OK_RESPONSE);		
					break;
				default:
					pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
					break;	
			}
			break;
		}
		case TLM_READ_COMMAND:
		{
			if(addr == DMA_S2MM_DMACR)
			{
				std::memcpy(data, &S2MM_DMACR, len);
				pl.set_response_status(TLM_OK_RESPONSE);
				break;
			}
			else
			{
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
				break;
			}
		}
		default:
			pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
			SC_REPORT_ERROR("IP", "TLM bad command");
		
	}
	offset += sc_time(5, SC_NS);
}


//Interpretacija tlm komunikacije prema ip modulu
void dma::b_transport_ip(pl_t& pl, sc_time& offset)
{
	tlm_command cmd     = pl.get_command();
	unsigned char* data = pl.get_data_ptr();
	uint32_t len	    = pl.get_data_length();
	uint64 addr         = pl.get_address();
	
	offset += sc_time(5, SC_NS);
	
	if(state == DMA_STREAM)
	{
		switch(cmd)
		{
			
			case TLM_WRITE_COMMAND:
			{ 	
				pl.set_address(S2MM_DA + transaction_number);
				isoc_ic->b_transport(pl, offset);
				transaction_number++;
				if(transaction_number == S2MM_LENGTH)
				{
					state = END;
					transaction_number = 0;
				}
				break;
			}
			case TLM_READ_COMMAND:
			{
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
				break;
			}
			default:
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
				SC_REPORT_ERROR("IP", "TLM bad command");
		}
		pl.set_address(addr);
	}
}

//metod dma kontrolera
void dma::dma_method()
{
 
	switch(state)
	{
		case RESET:
			dma_interrupt.write(false);
			transaction_number = 0; 
				
			state = IDLE;
			SC_REPORT_INFO("DMA", "state: RESET");
			S2MM_DMACR &= 0xFFFFFFFB;
			break;
		case DMA_STREAM:
			dma_interrupt.write(false);
			SC_REPORT_INFO("DMA", "state: STREAM");
			break;
		case END:
			SC_REPORT_INFO("DMA", "state: END");
			dma_interrupt.write(true);
			break;	
		case IDLE:
			state = IDLE;
			SC_REPORT_INFO("DMA", "state: IDLE");
			dma_interrupt.write(false);
			break;			
	}	
}

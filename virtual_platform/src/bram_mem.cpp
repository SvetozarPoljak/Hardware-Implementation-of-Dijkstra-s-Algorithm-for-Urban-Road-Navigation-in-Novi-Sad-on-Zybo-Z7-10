#include "bram_mem.hpp"
#include <tlm>

using namespace sc_core;
using namespace sc_dt;
using namespace std;
using namespace tlm;

void bram_mem::load_data_for_bram()
{                        
    	ifstream tail_file("../data/tail.dat", ios::binary);
    	ifstream head_file("../data/head.dat", ios::binary);
    	ifstream weight_file("../data/weight.dat", ios::binary);

    	tail_file.read(reinterpret_cast<char*>(tail), sizeof(unsigned) * ARC_NUM);
    	head_file.read(reinterpret_cast<char*>(head), sizeof(unsigned) * ARC_NUM);
    	weight_file.read(reinterpret_cast<char*>(edge_weights), sizeof(unsigned) * ARC_NUM);
}


//Konstruktor objekta klase bram_mem
bram_mem::bram_mem(sc_module_name name):
	sc_module(name),
	soc_ip("bram_soc_ip"),
	soc_ic("bram_soc_ic")
{
	soc_ip.register_b_transport(this, &bram_mem::b_transport_ip);
	soc_ic.register_b_transport(this, &bram_mem::b_transport_ic);
	
	load_data_for_bram();
	std::ofstream bram_file("bram_file.txt", std::ios::app);
	//head, tail i weight su vrednosti koje opisuju graf Novog Sada i taj graf se nece menjati
	for(int i = HEAD_AND_TAIL_BASE_ADDR; i != WEIGHT_BASE_ADDR; ++i)
	{
		bram[i] = (head[i] << 18) | (tail[i]);
		bram_file << bram[i] << ",\n";
	}
	
	for(int i = WEIGHT_BASE_ADDR; i != COST_AND_VISITED_VERTEX_BASE_ADDR; ++i)
	{
		bram[i] = edge_weights[i - WEIGHT_BASE_ADDR];
		bram_file << bram[i] << ",\n";
	}
	
	//cost i vertex_is_visited vrednosti su poznate tek kada se pronadju pocetno i krajnje teme trenutnog upita
	for(int i = COST_AND_VISITED_VERTEX_BASE_ADDR; i != BRAM_SIZE; ++i)
		bram[i] = 0;
}

//Modelovanje tlm komunikacije ka INTERCONNECT modulu
void bram_mem::b_transport_ic(pl_t& pl, sc_time& offset)
{
	tlm_command cmd 	= pl.get_command();
	uint64 adr		= pl.get_address();
	unsigned char *buf 	= pl.get_data_ptr();
	uint32_t temp_bram_cell;
	
	switch(cmd)
	{
		case TLM_WRITE_COMMAND:
			if(adr >= COST_AND_VISITED_VERTEX_BASE_ADDR && adr < BRAM_SIZE)
			{
				std::memcpy(&temp_bram_cell, buf, sizeof(temp_bram_cell));
				bram[adr] = temp_bram_cell;
				pl.set_response_status(TLM_OK_RESPONSE);
			}
			else
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
			break;
		case TLM_READ_COMMAND:
			if(adr < BRAM_SIZE)
			{
				temp_bram_cell = bram[adr];
				std::memcpy(buf, &temp_bram_cell, sizeof(temp_bram_cell));
				pl.set_response_status(TLM_OK_RESPONSE);
			}
			else
			{
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);	
			}
			break;
		default:
			pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
	}
	
	offset += sc_time(5, SC_NS);
}

//Modelovanje tlm komunikacije ka IP modulu
void bram_mem::b_transport_ip(pl_t& pl, sc_time& offset)
{
	tlm_command cmd 	= pl.get_command();
	uint64 adr		= pl.get_address();
	unsigned char *buf 	= pl.get_data_ptr();
	uint32_t temp_bram_cell;
	
	switch(cmd)
	{
		case TLM_WRITE_COMMAND:
			if(adr >= COST_AND_VISITED_VERTEX_BASE_ADDR && adr < BRAM_SIZE)
			{
				std::memcpy(&temp_bram_cell, buf, sizeof(temp_bram_cell));
				bram[adr] = temp_bram_cell;
				pl.set_response_status(TLM_OK_RESPONSE);
			}
			else
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
			break;
		case TLM_READ_COMMAND:
			if(adr < BRAM_SIZE)
			{
				temp_bram_cell = bram[adr];
				std::memcpy(buf, &temp_bram_cell, sizeof(temp_bram_cell));
				pl.set_response_status(TLM_OK_RESPONSE);
			}
			else
			{
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);	
			}
			break;
		default:
			pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
	}
	
	offset += sc_time(5, SC_NS);
}

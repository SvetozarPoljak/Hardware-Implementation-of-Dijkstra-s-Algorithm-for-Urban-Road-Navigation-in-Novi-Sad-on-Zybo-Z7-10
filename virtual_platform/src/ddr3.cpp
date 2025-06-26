#include "ddr3.hpp"

using namespace sc_core;
using namespace sc_dt;
using namespace tlm;

void ddr3_ram::load_data_for_ddr3()
{               
    	ifstream graph_latitudes_file("../data/graph_latitudes.dat", ios::binary);
    	ifstream graph_longitudes_file("../data/graph_longitudes.dat", ios::binary);             

    	graph_latitudes_file.read(reinterpret_cast<char*>(graph_latitudes), sizeof(float) * VERTEX_NUM);
    	graph_longitudes_file.read(reinterpret_cast<char*>(graph_longitudes), sizeof(float) * VERTEX_NUM);
}

/*	
	Konstruktor objekta klase ddr3_ram
 	Upis fiksnih podataka u RAM - ucitavanje podataka o mrezi Novog Sada u globalne nizove graph_latitudes, graph_longitudes 
 	pomocu load_data_for_ddr3();
*/ 
ddr3_ram::ddr3_ram(sc_module_name name):
	sc_module(name),
	soc_ic("ram_soc_ic")
{
	soc_ic.register_b_transport(this, &ddr3_ram::b_transport_ic);
	load_data_for_ddr3();
}

//Modelovanje tlm komunikacije
void ddr3_ram::b_transport_ic(pl_t& pl, sc_time& offset)
{
	tlm_command cmd 	= pl.get_command();
	uint64 adr		= pl.get_address();
	unsigned char *buf 	= pl.get_data_ptr();
	uint32_t len 		= pl.get_data_length();
	uint32_t integer_temp_ram_cell;
	float 	 float_temp_ram_cell;
	
	if(adr >= RAM_ADDR_H)
	{
		SC_REPORT_ERROR("DDR3", "Invalid address!");
		pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
		return;
	}
	
	switch(cmd)
	{
		case TLM_WRITE_COMMAND:
			if(adr > LAST_GRAPH_LONGITUDE_ADDR){
				std::memcpy(&integer_temp_ram_cell, buf, len);
				if(adr == LAST_GRAPH_LONGITUDE_ADDR + 1)
					relaxation_history.clear();	
				relaxation_history.insert(relaxation_history.begin(), integer_temp_ram_cell);
				pl.set_response_status(TLM_OK_RESPONSE);
			}
			else
			{
				SC_REPORT_ERROR("DDR3", "Invalid address!");
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
				return;
			}
			break;
		case TLM_READ_COMMAND:
			if(adr <= LAST_GRAPH_LONGITUDE_ADDR)
			{
				if(adr <= LAST_GRAPH_LATITUDE_ADDR)
					float_temp_ram_cell = graph_latitudes[adr];
				else
					float_temp_ram_cell = graph_longitudes[adr - LAST_GRAPH_LATITUDE_ADDR - 1];
				std::memcpy(buf, &float_temp_ram_cell, sizeof(float_temp_ram_cell));
				pl.set_response_status(TLM_OK_RESPONSE);
				break;				
			}
			else if(adr <= LAST_GRAPH_LONGITUDE_ADDR + relaxation_history.size())
			{
				integer_temp_ram_cell = relaxation_history[adr - LAST_GRAPH_LONGITUDE_ADDR - 1];
				std::memcpy(buf, &integer_temp_ram_cell, sizeof(integer_temp_ram_cell));
				pl.set_response_status(TLM_OK_RESPONSE);
				break;
			}
			else
			{
				SC_REPORT_ERROR("DDR3", "Invalid address!");
				pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
				return;
			}
		default:
			pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
	}
	
	//veci offset, jer je pristup ram memoriji sporije
	offset += sc_time(10, SC_NS);
}

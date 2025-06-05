#include "ip.hpp"
#include <systemc>
#include <string>
#include <sstream>
#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>

using namespace sc_core;
using namespace sc_dt;
using namespace std;
using namespace tlm;

SC_HAS_PROCESS(ip);

//Konstruktor objekta klase ip
ip::ip(sc_module_name name):
	sc_module(name),
	soc_ic("ip_soc_ic"),
	isoc_bram("ip_isoc_bram"),
	isoc_dma("ip_isoc_dma"),
	ip_interrupt("ip_interrupt")
{
	soc_ic.register_b_transport(this, &ip::b_transport_ic);
	SC_THREAD(relaxation);
	dont_initialize();
	sensitive << register_state_updated;
	
	ip_cfg_reg = 0x00;
	for(unsigned i = 0; i < 50; i++)
		relaxation_buffer[i] = 0; 
	index = 0;
}

//Interpretacija tlm komunikacije prema interconnect modulu
void ip::b_transport_ic(pl_t& pl, sc_time& offset)
{
	tlm_command cmd     = pl.get_command();
	uint64 addr	    = pl.get_address();
	unsigned char* data = pl.get_data_ptr();
	uint32_t len	    = pl.get_data_length();
		
	switch(cmd)
	{
		
		case TLM_WRITE_COMMAND:
		{
			std::memcpy(&ip_cfg_reg, data, len);
			pl.set_response_status(TLM_OK_RESPONSE);
			register_state_updated.notify(SC_ZERO_TIME);
			break;
		}
		case TLM_READ_COMMAND:
		{
			std::memcpy(data, &ip_cfg_reg, len);
			pl.set_response_status(TLM_OK_RESPONSE);
			break;
		}
		default:
			pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
			SC_REPORT_ERROR("IP", "TLM bad command");
	}
	
	offset += sc_time(5, SC_NS);
}

//proces koji modeluje proces relaksacije
void ip::relaxation()
{
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	
	//komunikacija
	pl_t pl;
	unsigned char data[sizeof(uint32_t)];
	unsigned char dma_stream_data[50 * sizeof(uint32_t)];
	uint32_t bram_cell_32bit, temp_32_bit;
	
	stringstream ss;
	
	while(1)
	{
		sc_time offset;
		if(ip_cfg_reg & 0x01)					//IP_RESET_BIT je 0. bit
		{	
			SC_REPORT_INFO("IP", "IP state: RESET");				
			state = RESET;
		}
		else if(ip_cfg_reg & 0x08)				//IP_DMA_STREAM_BIT je 3. bit
		{
			SC_REPORT_INFO("IP", "IP state: DMA_STREAM");			
			state = DMA_STREAM;
		}
		else if(ip_cfg_reg & 0x02 && !(ip_cfg_reg & 0x04))	//IP_DO_RELAXATION_BIT je 1. bit ali ne sme raditi relaksacije dok je poslat zahtev za stream, IP_DMA_REQ_BIT je 2. bit
		{	 
			SC_REPORT_INFO("IP", "IP state: DO_RELAXATION");
			state = DO_RELAXATION;
		}
		else if(ip_cfg_reg & 0x10)				//IP_END_BIT je 4. bit
		{
			SC_REPORT_INFO("IP", "IP state: END");
			state = END;
		}
		else
		{
			SC_REPORT_INFO("IP", "IP state: IDLE");
			state = IDLE;
		}	
		switch(state)
		{
			case RESET:
			{
				//pomocni bafer, koji drzi poslednjih 50 relaksacija se prazni
				for(i = 0; i < 50; i++)
					relaxation_buffer[i] = 0;
				index = 0;
				
				//vrednost signala je 0
				ip_interrupt.write(false);	

				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);	 
				
				//reset je ispunjen
				ip_cfg_reg = 0x00;
				break;
			}
			case DO_RELAXATION:
			{
				pl.set_data_ptr(data);
				//vrednost signala je 0
				ip_interrupt.write(false);
			
				min_cost = inf_weight;
				id_of_min_cost_vertex = 0;
				there_are_unvisited_vertices_in_table = false;
				all_costs_are_inf = true;
				next_iteration = false;
				
				for(i = 0; i < VERTEX_NUM; ++i)
				{
					//citanje podataka cost_from_start_vertex i vertex_is_visited iz BRAM modula i njihovo smestanje u promenljive temp_vertex_is_visited i temp_cost_from_start_vertex; 
					pl.set_address(COST_AND_VISITED_VERTEX_BASE_ADDR + i);		
					pl.set_command(TLM_READ_COMMAND);
					pl.set_data_length(sizeof(bram_cell_32bit));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_bram->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
					
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					temp_vertex_is_visited = bram_cell_32bit & 0x80000000;
					temp_cost_from_start_vertex = bram_cell_32bit & 0x7FFFFFFF;						
					
					//pronalazenje neposecenog temena sa minimalnom udaljenoscu od pocetnog temena
					if(temp_vertex_is_visited == false)
					{
						if(min_cost > temp_cost_from_start_vertex)
						{
							min_cost = temp_cost_from_start_vertex;
							id_of_min_cost_vertex = i;
						}
						if(all_costs_are_inf){
							if(temp_cost_from_start_vertex < inf_weight)
							{
								all_costs_are_inf = false;
							}
						}
					
						there_are_unvisited_vertices_in_table = true;
					}
					
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);			
				}
				
				//ukoliko su sva temena sa beskonacnom udaljenoscu ili ukoliko su sva temena posecena zavrsava se proces relaksacije
				if(there_are_unvisited_vertices_in_table == false || all_costs_are_inf == true)
				{
					//kraj procesa se upisuje u ip_cfg_reg registar
					ip_cfg_reg |= 0x10;				//IP_END_BIT je 4. bit
					
					if(index != 0)
					{
						ip_cfg_reg |= 0x04;			//IP_DMA_REQ_BIT je 2. bit, a IP_DMA_STREAM_BIT je 3.bit
						ip_cfg_reg &= 0xF7;		
					}	
					ip_interrupt.write(true);
					
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					break;
				}

				/****************************************************************************************************************************************
					Za trenutno najblize neposeceno teme se proverava da li je moguca relaksacija njegovih suseda.
					Relaksacija se izvrsava ukoliko je moguca, teme se izbacuje iz liste neposecenih temena, a 
					relaksacija se upisuje u pomocni bafer, u 32 bitni kontejner u formatu relaksairajuce teme(31-18 bit) relaksirano teme(13-0 bit) 
				****************************************************************************************************************************************/
				i = 0;
				pl.set_address(HEAD_AND_TAIL_BASE_ADDR + i);		
				pl.set_command(TLM_READ_COMMAND);
				pl.set_data_length(sizeof(bram_cell_32bit));
				pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
				isoc_bram->b_transport(pl, offset);
				assert(pl.get_response_status() == TLM_OK_RESPONSE);
				std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
				
				temp_tail = bram_cell_32bit & 0x00003FFF;
				
				//Prolazak kroz lokacije u BRAM-u na kojima su tail vrednosti u potrazi za ID vrednosti neposecenog temena trenutno najblizeg pocetnom temenu
				//Ukoliko ga nema, prelazi se na sledecu iteraciju
				while(i < ARC_NUM && temp_tail != id_of_min_cost_vertex)
				{
					i++;
					
					if (i == ARC_NUM){
						bram_cell_32bit = 0x80000000;
						pl.set_address(COST_AND_VISITED_VERTEX_BASE_ADDR + id_of_min_cost_vertex);		
						pl.set_command(TLM_WRITE_COMMAND);
						pl.set_data_length(sizeof(bram_cell_32bit));
						std::memcpy(data, &bram_cell_32bit, sizeof(bram_cell_32bit));
						pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
						isoc_bram->b_transport(pl, offset);
						assert(pl.get_response_status() == TLM_OK_RESPONSE);
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						next_iteration = true;
				    		break;
				    	}
				    	pl.set_address(HEAD_AND_TAIL_BASE_ADDR + i);		
					pl.set_command(TLM_READ_COMMAND);
					pl.set_data_length(sizeof(bram_cell_32bit));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_bram->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					temp_tail = bram_cell_32bit & 0x00003FFF;
				}
				if(next_iteration)
				{
					next_iteration = false;
					break;
				}
				
				//Ukoliko je ID vrednost pronadjena u tail vrednostima ispituju se sva susedna temena pretragom head vrednosti u BRAMU
				//u potrazi za potencijalnom relaksacijom 		
				while(i < ARC_NUM && temp_tail == id_of_min_cost_vertex)
				{						
				    	pl.set_address(HEAD_AND_TAIL_BASE_ADDR + i);		
					pl.set_command(TLM_READ_COMMAND);
					pl.set_data_length(sizeof(bram_cell_32bit));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_bram->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					temp_head = (bram_cell_32bit >> 18) & 0x00003FFF;
					
				    	pl.set_address(COST_AND_VISITED_VERTEX_BASE_ADDR + temp_head);		
					pl.set_command(TLM_READ_COMMAND);
					pl.set_data_length(sizeof(bram_cell_32bit));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_bram->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					temp_cost_from_start_vertex = bram_cell_32bit & 0x7FFFFFFF;			
					
				    	pl.set_address(WEIGHT_BASE_ADDR + i);		
					pl.set_command(TLM_READ_COMMAND);
					pl.set_data_length(sizeof(bram_cell_32bit));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_bram->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					temp_edge_weight = bram_cell_32bit & 0x000FFFFF;			
					
					//Relaksacija je moguca				
					if(temp_cost_from_start_vertex > temp_edge_weight + min_cost)
					{	
					    	pl.set_address(COST_AND_VISITED_VERTEX_BASE_ADDR + temp_head);		
						pl.set_command(TLM_WRITE_COMMAND);
						bram_cell_32bit = temp_edge_weight + min_cost;
						pl.set_data_length(sizeof(bram_cell_32bit));
						std::memcpy(data, &bram_cell_32bit, sizeof(bram_cell_32bit));
						pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
						isoc_bram->b_transport(pl, offset);
						assert(pl.get_response_status() == TLM_OK_RESPONSE);
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);	
										
						node_id_t relaxing_vertex = id_of_min_cost_vertex;
						node_id_t relaxed_vertex = temp_head;
						
						uint32_t new_relaxation = (relaxing_vertex << 18) | (relaxed_vertex);
						relaxation_buffer[index] = new_relaxation;					 
					  	index++;
					  	if(index == 50)
						{	
							ip_cfg_reg |= 0x04;			//IP_DMA_REQ_BIT je 2. bit, a IP_DMA_STREAM_BIT je 3.bit
							ip_cfg_reg &= 0xF7;	
							SC_REPORT_INFO("IP", "Maksimalni kapacitet IP pomocnog bafera dostignut!");
							ip_interrupt.write(true);
							break;		
						}
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);	
					}	
					i++;	
					
					pl.set_address(HEAD_AND_TAIL_BASE_ADDR + i);		
					pl.set_command(TLM_READ_COMMAND);
					pl.set_data_length(sizeof(bram_cell_32bit));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_bram->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					std::memcpy(&bram_cell_32bit, data, sizeof(bram_cell_32bit));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					temp_tail = bram_cell_32bit & 0x00003FFF;	
				}
				
				//trenutno najblize neposeceno teme se uklanja iz tabele neposecenih temena	
				bram_cell_32bit = 0x80000000;
				std::memcpy(data, &bram_cell_32bit, sizeof(bram_cell_32bit));
				pl.set_address(COST_AND_VISITED_VERTEX_BASE_ADDR + id_of_min_cost_vertex);		
				pl.set_command(TLM_WRITE_COMMAND);
				pl.set_data_length(sizeof(bram_cell_32bit));
				pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
				isoc_bram->b_transport(pl, offset);				
				assert(pl.get_response_status() == TLM_OK_RESPONSE);
				
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
				break;
			}
			case DMA_STREAM:
			{	
				//vrednost signala je 0
				ip_interrupt.write(false);
				
				// magistrala ide samo ka DMA modulu
				pl.set_address(0);    
				pl.set_data_ptr(dma_stream_data);
				
				for(i = 0; i < 50; i++)
				{
					std::memcpy(dma_stream_data, &(relaxation_buffer[i]), sizeof(uint32_t));
					pl.set_command(TLM_WRITE_COMMAND);
					pl.set_data_length(sizeof(uint32_t));
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
					isoc_dma->b_transport(pl, offset);
					assert(pl.get_response_status() == TLM_OK_RESPONSE);
					relaxation_buffer[i] = 0;
				}
				index = 0;
				
				ip_cfg_reg &= 0xF7;
				
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
				SC_REPORT_INFO("IP", "Praznjenje IP pomocnog bafera gotovo!");
									
				break;
			}
			case END:
			{	
				//vrednost signala je 1
				ip_interrupt.write(true);
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);					
				break;
			}
			
			case IDLE:
			{	
				//vrednost signala je 0
				ip_interrupt.write(false);
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
				break;
			}
		}
	}
}

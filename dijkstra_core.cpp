#include "dijkstra_core.h"

DijkstraCore::DijkstraCore()
{
	for(int i = 0; i < 50; i++)
		relaxation_buffer[i] = 0;	
	buffer_index = 0;
	dma_index = 0;
	ip_cfg_reg = ap_uint<8>(0);
	ip_interrupt = false;
	dma_out = 0;
	state = IDLE;
}

void top_function(ap_uint<8> *cfg_val, ap_uint<32> bram[49208], bool *ip_interrupt, ap_uint<32> *dma_out)
{
	static DijkstraCore IP;

	//IP_RESET_BIT je 0. bit
	//IP_DO_RELAXATION_BIT je 1. bit 
	//ali ne sme raditi relaksacije dok je poslat zahtev za stream, 
	//IP_DMA_REQ_BIT je 2. bit
	//IP_DMA_STREAM_BIT je 3. bit
	//IP_END_BIT je 4. bit
	IP.ip_cfg_reg = *cfg_val;

	if(IP.ip_cfg_reg[0])
		IP.state = RESET;
	else if(IP.ip_cfg_reg[3])
		IP.state = DMA_STREAM;
	else if(IP.ip_cfg_reg[1] && !(IP.ip_cfg_reg[2]))
		IP.state = DO_RELAXATION;
	else if(IP.ip_cfg_reg[4])
		IP.state = END;
	else
		IP.state = IDLE;
	
	switch(IP.state)
	{
		case IDLE:
		{
			IP.ip_interrupt = false;
			break;
		}
		case RESET:
		{
			IP.ip_interrupt = false;
			reset_l:for(int i = 0; i < 50; i++)
				IP.relaxation_buffer[i] = 0;
			IP.buffer_index = 0;
			IP.dma_index = 0;
			IP.ip_cfg_reg = ap_uint<8>(0);
			break;
		}
		case DO_RELAXATION:
		{
			IP.ip_interrupt = false;
			ap_uint<31> min_cost = inf_weight;
			ap_uint<14> id_of_min_cost_vertex = 0;
			bool there_are_unvisited_vertices_in_table = false;
			bool all_costs_are_inf = true;
			ap_uint<31> temp_cost_from_start_vertex;
			ap_uint<14> temp_head;
			ap_uint<14> temp_tail;
			ap_uint<20> temp_edge_weight;

			l_1:for(int i = 0; i < VERTEX_NUM; ++i)
			{
				//citanje podataka cost_from_start_vertex i vertex_is_visited iz BRAM modula
				temp_cost_from_start_vertex = bram[COST_AND_VISITED_VERTEX_BASE_ADDR + i] & 0x7FFFFFFF;
				//pronalazenje neposecenog temena sa minimalnom udaljenoscu od pocetnog temena
				if(bram[COST_AND_VISITED_VERTEX_BASE_ADDR + i][31] == 0)
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
			}

			//ukoliko su sva temena sa beskonacnom udaljenoscu ili ukoliko su sva temena posecena zavrsava se proces relaksacije
			if(there_are_unvisited_vertices_in_table == false || all_costs_are_inf == true)
			{
				//IP_END_BIT je 4. bit
				IP.ip_cfg_reg.set(4, 1);
				if(IP.buffer_index != 0)
				{
					IP.ip_cfg_reg.set(2, 1);			//IP_DMA_REQ_BIT je 2. bit, a IP_DMA_STREAM_BIT je 3.bit
					IP.ip_cfg_reg.set(3, 0);
				}
				IP.ip_interrupt = true;
				break;
			}

			/****************************************************************************************************************************************
				Za trenutno najblize neposeceno teme se proverava da li je moguca relaksacija njegovih suseda.
				Relaksacija se izvrsava ukoliko je moguca, teme se izbacuje iz liste neposecenih temena, a
				relaksacija se upisuje u pomocni bafer, u 32 bitni kontejner u formatu relaksairajuce teme(31-18 bit) relaksirano teme(13-0 bit)
			****************************************************************************************************************************************/
			int start_idx = -1;
			//Prolazak kroz lokacije u BRAM-u na kojima su tail vrednosti u potrazi za ID vrednosti neposecenog temena trenutno najblizeg pocetnom temenu
			//Ukoliko ga nema, prelazi se na sledecu iteraciju
			l_2:for(int i = 0; i < ARC_NUM; ++i)
			{
				temp_tail = bram[HEAD_AND_TAIL_BASE_ADDR + i] & 0x3FFF;
				if(temp_tail == id_of_min_cost_vertex)
				{
					start_idx = i;
					break;
				}
			}
			if (start_idx < 0)
			{
   				bram[COST_AND_VISITED_VERTEX_BASE_ADDR + id_of_min_cost_vertex].set(31, 1);
    				break;
			}

			//Ukoliko je ID vrednost pronadjena u tail vrednostima ispituju se sva susedna temena pretragom head vrednosti u BRAMU
			//u potrazi za potencijalnom relaksacijom
			l_3:for(int j = start_idx; j < ARC_NUM; ++j)
			{
				temp_tail = bram[HEAD_AND_TAIL_BASE_ADDR + j] & 0x00003FFF;
				if(temp_tail != id_of_min_cost_vertex)
					break;

				temp_head = (bram[HEAD_AND_TAIL_BASE_ADDR + j] >> 18) & 0x3FFF;
				temp_cost_from_start_vertex = bram[COST_AND_VISITED_VERTEX_BASE_ADDR + temp_head] & 0x7FFFFFFF;
				temp_edge_weight = bram[WEIGHT_BASE_ADDR + j] & 0xFFFFF;

				//Relaksacija je moguca
				if(temp_cost_from_start_vertex > temp_edge_weight + min_cost)
				{
					bram[COST_AND_VISITED_VERTEX_BASE_ADDR + temp_head] = temp_edge_weight + min_cost;

					ap_uint<14> relaxing_vertex = id_of_min_cost_vertex;
					ap_uint<14> relaxed_vertex = temp_head;

					ap_uint<32> new_relaxation = (ap_uint<32>(relaxing_vertex) << 18) | relaxed_vertex;

					IP.relaxation_buffer[IP.buffer_index++] = new_relaxation;

					if(IP.buffer_index == 50)
					{
						//IP_DMA_REQ_BIT je 2. bit, a IP_DMA_STREAM_BIT je 3.bit
						IP.ip_cfg_reg.set(2, 1);
						IP.ip_cfg_reg.set(3, 0);
						IP.ip_interrupt = true;
						break;
					}
				}
			}
			//trenutno najblize neposeceno teme se uklanja iz tabele neposecenih temena
			bram[COST_AND_VISITED_VERTEX_BASE_ADDR + id_of_min_cost_vertex].set(31, 1);
			break;
		}
		case DMA_STREAM:
		{
			IP.ip_interrupt = false;
			if(IP.dma_index < 50)
			{
				IP.dma_out = IP.relaxation_buffer[IP.dma_index];
				IP.relaxation_buffer[IP.dma_index] = 0;
				IP.dma_index++;
			}
			else
			{
				IP.dma_index = 0;
		       	IP.buffer_index = 0;
				IP.ip_cfg_reg.set(3, 0);
			}
			break;
		}
		case END:
		{
			IP.ip_interrupt = true;
			break;
		}
	}

	*cfg_val = IP.ip_cfg_reg;
	*ip_interrupt = IP.ip_interrupt;
	*dma_out = IP.dma_out;
}

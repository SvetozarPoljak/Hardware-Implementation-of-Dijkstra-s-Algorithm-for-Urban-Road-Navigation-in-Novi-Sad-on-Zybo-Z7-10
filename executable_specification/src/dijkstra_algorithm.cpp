#include "dijkstra_algorithm.hpp"

/***************************************************PATH************************************************************/
int addToPath(PATH **phead, unsigned new_ID)
{
	PATH *new_el, *temp;
	new_el = (PATH *)malloc(sizeof(PATH));
	if(new_el == NULL)
		return 0;
		
	new_el -> ID = new_ID;
	new_el -> next = *phead;
	*phead = new_el;
	
	return 1;
}

int deleteLastPathEl(PATH **phead)
{
	PATH *del;
	if((*phead) == NULL)
		return 0;
	del = (*phead);
	(*phead) = (*phead) -> next;
	free(del);
	del = NULL;
	
	return 1;
}

void deletePath(PATH **phead)
{
	while((*phead) != NULL)
	{
		deleteLastPathEl(phead);
	}
	std::cout<<"Path deleted"<<std::endl;
}
/**************************************************END_OF_PATH**************************************************/

/*********************************************RELAXATION**************************************************************/
int addToRelaxationHistory(RELAXATION **phead, unsigned new_relaxing, unsigned new_relaxed)
{

	RELAXATION *new_el, *temp;
	new_el = (RELAXATION *)malloc(sizeof(RELAXATION));
	if(new_el == NULL)
		return 0;
		
	new_el -> relaxing = new_relaxing;
	new_el -> relaxed = new_relaxed;
	new_el -> next = *phead;
	*phead = new_el;
	
	return 1;
}

int deleteLastRelaxation(RELAXATION **phead)
{
	RELAXATION *del;
	if((*phead) == NULL)
		return 0;
	del = (*phead);
	(*phead) = (*phead) -> next;
	free(del);
	del = NULL;
	
	return 1;
}

void deleteRelaxationHistory(RELAXATION **phead)
{
	while((*phead) != NULL)
	{
		deleteLastRelaxation(phead);
	}
	std::cout<<"Relaxation history deleted"<<std::endl;
}

// Ovo je nas IP. On izbacuje izvrsene relaksacije u glavnu memoriju. U procesu relaksacije koristi se podacima iz BRAMA koji opisuju mrezu Novog Sada. 
int relaxationProcess(RELAXATION **prel_head, unsigned *cost_from_start_vertex, bool *vertex_is_visited, unsigned *head, unsigned *tail, unsigned *edge_weights)
{	
	unsigned min_cost = inf_weight;
	unsigned id_of_min_cost_vertex = 0;
	bool there_are_unvisited_vertices_in_table = false;
	bool all_costs_are_inf = true;
	unsigned i, temp_id;
	
	for(i = 0; i < VERTEX_NUM; ++i)
	{
		if(vertex_is_visited[i] == false)
		{
			if(min_cost > cost_from_start_vertex[i])
			{
				min_cost = cost_from_start_vertex[i];
				id_of_min_cost_vertex = i;
			}
			
			if(all_costs_are_inf){
				if(cost_from_start_vertex[i] < inf_weight)
				{
					all_costs_are_inf = false;
				}
			}
			
			there_are_unvisited_vertices_in_table = true;
		}
	}
		
	if(there_are_unvisited_vertices_in_table == false)
		return 0;
		
	if(all_costs_are_inf == true)
		return 0;
	
	i = 0;
	while(i < ARC_NUM && tail[i] != id_of_min_cost_vertex)
	{
		i++;
		if (i == ARC_NUM){
			vertex_is_visited[id_of_min_cost_vertex] = true;
            		return 1;
            	}
	}
	
	while(i < ARC_NUM && tail[i] == id_of_min_cost_vertex)
	{		
		temp_id = head[i];
		if(cost_from_start_vertex[temp_id] > edge_weights[i] + min_cost)
		{		
			cost_from_start_vertex[temp_id] = edge_weights[i] + min_cost;
			addToRelaxationHistory(prel_head, id_of_min_cost_vertex, temp_id);
		}		
		i++;
	}
	
	/*
		1. dodaj element u istoriju relaksacija
		2. "ukloni" ovaj element iz tabele
	*/
	vertex_is_visited[id_of_min_cost_vertex] = true;
	return 1;
}

/*******************************************************END_OF_RELAXATION****************************************************************************/

/********************************************************DIJKSTRA'S_OUTPUT********************************************************************************/
void parsePath(RELAXATION **prel_head, unsigned start_vertex, unsigned end_vertex, PATH **ppath_head)
{
	if(start_vertex == end_vertex)
	{
		std::cout<<"-----------------------------------------"<<std::endl;
		std::cout<<"Pocetna i krajnja lokacija su ista tacka."<<std::endl;
		std::cout<<"-----------------------------------------"<<std::endl;
		return;
	}
	RELAXATION *temp = *prel_head;
	while(temp != NULL && temp -> relaxed != end_vertex)
		temp = temp -> next;
	if(temp == NULL)
	{
		std::cout<<"-------------------------------------------------"<<std::endl;
		std::cout<<"Saobracajnica izmedju unetih lokacija ne postoji."<<std::endl;
		std::cout<<"-------------------------------------------------"<<std::endl;
		return;
	}	
	unsigned this_vertex = temp -> relaxed;
	unsigned got_relaxed_by = temp -> relaxing;
	addToPath(ppath_head, this_vertex);
	addToPath(ppath_head, got_relaxed_by);
	while(got_relaxed_by != start_vertex)
	{
		while(got_relaxed_by != temp -> relaxed)
			temp = temp -> next;
		this_vertex = temp -> relaxed;
		got_relaxed_by = temp -> relaxing;
		addToPath(ppath_head, got_relaxed_by);
	}
	
	std::cout<<"-------------"<<std::endl;
	std::cout<<"Srecan put :)"<<std::endl;
	std::cout<<"-------------"<<std::endl;
}
/**********************************************************************************************************************************************************/

#ifndef DIJKSTRA_ALGORITHM_HPP
#define DIJKSTRA_ALGORITHM_HPP

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
/*****************************************CONSTANTS_AND_ASSUMPTIONS*********************************************/
#include "constants.h"
#define VERTEX_NUM 9122
#define ARC_NUM 20043
/***************************************************************************************************************/

/*************************************************PATH**********************************************************/

/*

	Struktura predstavlja jednog clana optimalne putanje Dijkstra algoritma

*/

typedef struct path_st
{
	unsigned ID;
	struct path_st *next;
}PATH;

/*

	- Dodavanje u listu jednog clana optimalne putanje Dijkstra algoritma

*/
int addToPath(PATH **phead, unsigned new_ID);

/*

	- Brisanje poslednjeg dodatog clana optimalne putanje Dijkstra algoritma

*/
int deleteLastPathEl(PATH **phead);

/*

	- Brisanje cele liste istorije relaksacija

*/
void deletePath(PATH **phead);

/**************************************************END_OF_PATH**************************************************/

/**********************************************************************RELAXATION*************************************************************************/

/*

	Struktura predstavlja jednog clana istorije relaksacija

*/

typedef struct relaxation_st
{
	unsigned relaxing;
	unsigned relaxed;
	struct relaxation_st *next;
}RELAXATION;

/*

	- Dodavanje u listu koja reprezentuje istoruju relaksacija

*/
int addToRelaxationHistory(RELAXATION **phead, unsigned new_relaxing, unsigned new_relaxed);

/*

	- Brisanje poslednje relaksacije

*/
int deleteLastRelaxation(RELAXATION **phead);

/*

	- Brisanje cele liste istorije relaksacija

*/
void deleteRelaxationHistory(RELAXATION **phead);

/*

	- Srce algoritma, obilazak po temenima grafa i relaksacije temena

*/

int relaxationProcess(RELAXATION **prel_head, unsigned *cost_from_start_vertex, bool *vertex_is_visited, unsigned *head, unsigned *tail, unsigned *edge_weights);

/*******************************************************END_OF_RELAXATION*********************************************************************************/

/********************************************************DIJKSTRA'S_OUTPUT********************************************************************************/
/*

	- Prikazivanje puta

*/
void parsePath(RELAXATION **prel_head, unsigned start_vertex, unsigned end_vertex, PATH **ppath_head);
/********************************************************************************************************************************************************/

#endif

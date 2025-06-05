#include "geo_position_to_node.h"
#include "dijkstra_algorithm.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

using namespace std;

float graph_latitudes[VERTEX_NUM];
float graph_longitudes[VERTEX_NUM];
unsigned head[ARC_NUM];
unsigned tail[ARC_NUM];
unsigned edge_weights[ARC_NUM];
unsigned cost_from_start_vertex[VERTEX_NUM];
bool vertex_is_visited[VERTEX_NUM];

void load_data(); 

int main() {
    
    load_data();
    
    RELAXATION *rel_head = NULL;
    PATH *path_head = NULL;
    
    ifstream input_file("src/uploads/input_file.txt");
    if (!input_file.is_open()) {
        cerr << "Nije moguće otvoriti ulazni fajl!" << endl;
        return -1;
    }

    float start_lat, start_lon, end_lat, end_lon;
    input_file >> start_lat >> start_lon >> end_lat >> end_lon;
    
    unsigned start_vertex = naive_approach(graph_latitudes, graph_longitudes, start_lat, start_lon, 1500, VERTEX_NUM);
    unsigned end_vertex = naive_approach(graph_latitudes, graph_longitudes, end_lat, end_lon, 1500, VERTEX_NUM);

    if (start_vertex == invalid_id || end_vertex == invalid_id) {
        cerr << "Nije moguće pronaći čvor u radijusu od 1500m za početnu ili krajnju tačku." << endl;
        return -1;
    }

    for(unsigned i = 0; i < VERTEX_NUM; ++i)
    {
	if(i == start_vertex)
		cost_from_start_vertex[i] = 0;
	else
		cost_from_start_vertex[i] = inf_weight;
	vertex_is_visited[i] = false;
    }

    auto start = std::chrono::high_resolution_clock::now();
    unsigned relaxation_process_is_not_finished = 1;
    while(relaxation_process_is_not_finished)
	relaxation_process_is_not_finished = relaxationProcess(&rel_head, cost_from_start_vertex, vertex_is_visited, head, tail, edge_weights);
    cout<<endl<<"...DONE!"<<endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Vreme izvršavanja procesa relaksacija je: " << duration.count() << " s." << std::endl;
    
    parsePath(&rel_head, start_vertex, end_vertex, &path_head);

    ofstream output_file("src/output_file.txt");
    if (!output_file.is_open()) {
        cerr << "Nije moguće otvoriti izlazni fajl!" << endl;
        return -1;
    }

    PATH *temp = path_head;
    std::cout << "ID numbers of path: ";
    while(temp != NULL) {
    	std::cout << temp -> ID << " ";
        float lat = graph_latitudes[temp -> ID];
        float lon = graph_longitudes[temp -> ID];
        output_file << lat << " " << lon << endl;
        temp = temp -> next;
    }

    output_file.close();

    deleteRelaxationHistory(&rel_head);
    deletePath(&path_head);
	
    return 0;
}

void load_data()
{        
               
    ifstream graph_latitudes_file("data/graph_latitudes.dat", ios::binary);
    ifstream graph_longitudes_file("data/graph_longitudes.dat", ios::binary);             

    graph_latitudes_file.read(reinterpret_cast<char*>(graph_latitudes), sizeof(float) * VERTEX_NUM);
    graph_longitudes_file.read(reinterpret_cast<char*>(graph_longitudes), sizeof(float) * VERTEX_NUM);
    

    ifstream tail_file("data/tail.dat", ios::binary);
    ifstream head_file("data/head.dat", ios::binary);
    ifstream weight_file("data/weight.dat", ios::binary);

    tail_file.read(reinterpret_cast<char*>(tail), sizeof(unsigned) * ARC_NUM);
    head_file.read(reinterpret_cast<char*>(head), sizeof(unsigned) * ARC_NUM);
    weight_file.read(reinterpret_cast<char*>(edge_weights), sizeof(unsigned) * ARC_NUM);
}

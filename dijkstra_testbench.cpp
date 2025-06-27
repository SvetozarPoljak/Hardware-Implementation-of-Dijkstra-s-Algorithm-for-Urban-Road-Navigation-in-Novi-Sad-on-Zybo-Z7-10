#include <iostream>
#include <fstream>
#include <string>
#include <ap_int.h>
#include "dijkstra_core.h"

const int SIZE = 9400;
ap_uint<32> exp_result[SIZE];
ap_uint<32> bram[49208];

void load_exp_result(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Ne mogu da otvorim fajl: " << filename << std::endl;
        return;
    }

    unsigned int value;
    for (int i = 0; i < SIZE; ++i) {
        file >> value;
        exp_result[i] = value;
        if (file.peek() == ',')
        	file.ignore();
    }

    file.close();
}

void load_bram(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Ne mogu da otvorim fajl: " << filename << std::endl;
        return;
    }

    unsigned int value;
    for (int i = 0; i < 49208; ++i) {
        file >> value;
        bram[i] = value;
        if (file.peek() == ',')
        	file.ignore();
    }

    file.close();
}

int main(int argc, char ** argv)
{
	load_exp_result("exp_file.txt");
	load_bram("bram_file.txt");
	
	ap_uint<32> dma_out;
	bool ip_interrupt;
	ap_uint<8> cfg_val = ap_uint<8>(0x00);

	int err_cnt = 0;

	bool done = false;
	ap_uint<32> hw_result[9400];
	cfg_val.set(1, 1);
	int i = 0;
	
	int iter_num = 0;

	do
	{
		std::cout << "testing..." << std::endl;
		#ifdef HW_COSIM
		top_function(&cfg_val, bram, &ip_interrupt, &dma_out);
		#endif
		
		if(cfg_val[3])
		{
			std::cout << "DMA out[" << i << "] = " << dma_out.to_uint() << std::endl;
			hw_result[i] = dma_out;
			i++;
		}

		if(ip_interrupt && cfg_val[2])
		{
			std::cout << "DMA request sent!" << endl;
			cfg_val.set(2, 0);
			cfg_val.set(3, 1);
		}

		iter_num++;
	}
	while(!(ip_interrupt && cfg_val[4]));
	
	if(ip_interrupt && cfg_val[4])
		cout << "testing is done. there was " << iter_num << "iterations." << endl;

	for(int j = 0; j < 9400; j++)
		if(hw_result[j].to_uint() != exp_result[j].to_uint())
		{
			//cout << hw_result[j].to_uint() << " " << exp_result[j].to_uint() << endl;
			err_cnt++;
		}
	if(err_cnt)
 		cout << "ERROR: " << err_cnt << " mismatches detected!" << endl;
 	else
 		cout << "Test passes." << endl;
	
	return err_cnt;
}

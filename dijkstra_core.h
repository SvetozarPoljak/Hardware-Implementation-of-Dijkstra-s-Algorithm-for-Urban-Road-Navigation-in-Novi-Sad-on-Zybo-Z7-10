#ifndef _DIJKSTRA_CORE_HPP_
#define _DIJKSTRA_CORE_HPP_

#include "ap_int.h"
#include "hls_math.h"
#include "ap_shift_reg.h"

#define VERTEX_NUM 9122
#define ARC_NUM 20043

using namespace std;

#define HW_COSIM

//stanje IP modula
enum  ip_state_t {RESET, DMA_STREAM, DO_RELAXATION, END, IDLE};
const ap_uint<14> invalid_id = 0x00003FFF;
const ap_uint<31> inf_weight = 0x7FFFFFFF;

//kako je konfigurisan bram
const ap_uint<32> HEAD_AND_TAIL_BASE_ADDR = 0;
const ap_uint<32> WEIGHT_BASE_ADDR = 20043;
const ap_uint<32> COST_AND_VISITED_VERTEX_BASE_ADDR = 40086;

class DijkstraCore 
{
	public:
		ip_state_t state;    	
		ap_uint<32> relaxation_buffer[50];
		ap_uint<6> buffer_index;
		ap_uint<6> dma_index;
		bool ip_interrupt;
		ap_uint<8> ip_cfg_reg;
		ap_uint<32> dma_out;

		DijkstraCore();	
		
		void setCfgReg(ap_uint<8> cfg_val);
		ap_uint<8> getCfgReg();
		bool getInterruptVal();
		ap_uint<32> getDMAOut();
		ap_uint<6> getBufferIndex();
};

void top_function(ap_uint<8> *cfg_val, ap_uint<32> bram[49208], bool *ip_interrupt, ap_uint<32> *dma_out);

#endif

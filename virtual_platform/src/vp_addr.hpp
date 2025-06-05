#ifndef _VP_ADDR_HPP_
#define _VP_ADDR_HPP_

#include "bram_dma_ip_consts.hpp"
#include "ddr3_addr.hpp"

using namespace sc_dt;

//bazne adrese
const uint64 VP_ADDR_RAM  = 0x00000000;
const uint64 VP_ADDR_BRAM = 0xEE700000;
const uint64 VP_ADDR_IP   = 0xEE800000;
const uint64 VP_ADDR_DMA  = 0xEE900000;

//adrese RAM memorije
const uint64 VP_ADDR_LAST_GRAPH_LATITUDE_ADDR = VP_ADDR_RAM + LAST_GRAPH_LATITUDE_ADDR;
const uint64 VP_ADDR_LAST_GRAPH_LONGITUDE_ADDR = VP_ADDR_RAM + LAST_GRAPH_LONGITUDE_ADDR;
const uint64 VP_ADDR_RAM_H = RAM_ADDR_H;

//adrese pojedinacnih opsega BRAM memorije
const uint64 VP_ADDR_BRAM_HEAD_AND_TAIL  	    = VP_ADDR_BRAM + HEAD_AND_TAIL_BASE_ADDR;
const uint64 VP_ADDR_BRAM_WEIGHT		    = VP_ADDR_BRAM + WEIGHT_BASE_ADDR;
const uint64 VP_ADDR_BRAM_COST_AND_VISITED_VERTEX   = VP_ADDR_BRAM + COST_AND_VISITED_VERTEX_BASE_ADDR;
const uint64 VP_ADDR_BRAM_H			    = 0xEE70C038; // lokalne bram adrese idu od 0-49207, prva slobodna je 49208(0xC038)

//adrese registara IP modula
const uint64 VP_ADDR_IP_CFG_REG               	    = VP_ADDR_IP + IP_CFG_REG;  

//adrese registara DMA modula
const uint64 VP_ADDR_DMA_S2MM_DA     = VP_ADDR_DMA + DMA_S2MM_DA;
const uint64 VP_ADDR_DMA_S2MM_LENGTH = VP_ADDR_DMA + DMA_S2MM_LENGTH;
const uint64 VP_ADDR_DMA_S2MM_DMACR  = VP_ADDR_DMA + DMA_S2MM_DMACR;

#endif

#ifndef _BRAM_DMA_IP_CONSTS_HPP_
#define _BRAM_DMA_IP_CONSTS_HPP_

#include <systemc>

//kako je konfigurisan bram
const uint32_t HEAD_AND_TAIL_BASE_ADDR = 0;
const uint32_t WEIGHT_BASE_ADDR = 20043;
const uint32_t COST_AND_VISITED_VERTEX_BASE_ADDR = 40086;

//registar u IP-u
const uint32_t IP_CFG_REG = 0;

//DMA registri
const uint32_t DMA_S2MM_DMACR = 0x30;
const uint32_t DMA_S2MM_DA = 0x18;
const uint32_t DMA_S2MM_LENGTH = 0x28;

#endif

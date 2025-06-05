#ifndef _DDR3_ADDR_HPP_
#define _DDR3_ADDR_HPP_

#include "graph_dimensions.hpp"

using namespace sc_core; 
using namespace sc_dt;

const uint64 RAM_BASE_ADDR = 0x00000000;

const uint64 LAST_GRAPH_LATITUDE_ADDR = RAM_BASE_ADDR + (VERTEX_NUM - 1);
const uint64 LAST_GRAPH_LONGITUDE_ADDR = RAM_BASE_ADDR + (LAST_GRAPH_LATITUDE_ADDR + VERTEX_NUM); 

const uint64 RAM_ADDR_H = 0xEE6B281; //1 GB RAM

#endif


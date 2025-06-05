#ifndef _GRAPH_DIMENSIONS_HPP_
#define _GRAPH_DIMENSIONS_HPP_

#include <systemc>

typedef sc_dt::sc_uint<14> node_id_t;
typedef sc_dt::sc_uint<20> weight_t;
typedef sc_dt::sc_uint<31> cost_t;

//konstante grafa
const sc_dt::sc_uint<14> invalid_id = 0x00003FFF;
const sc_dt::sc_uint<31> inf_weight = 0x7FFFFFFF;
#define VERTEX_NUM 9122
#define ARC_NUM 20043

#endif

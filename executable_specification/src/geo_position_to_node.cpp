#include "geo_position_to_node.h"
#include "geo_dist.h"

#include <math.h>
#include <assert.h>

// Implementacija naive approach

	unsigned naive_approach(float *graph_latitudes, float *graph_longitudes, float user_node_latitude, float user_node_longitude, unsigned radius, unsigned vertex_count)
	{
		assert(radius >= 0.0 && "radius must be positive");
		float distance_to_pivot; //float distance_to_pivot[vertex_count];
		float min_distance = inf_weight;
		unsigned node_id = invalid_id;
		
		for(unsigned i = 0; i < vertex_count; i++)
		{
			distance_to_pivot = geo_dist(user_node_latitude, user_node_longitude, graph_latitudes[i], graph_longitudes[i]);
			if(distance_to_pivot < min_distance && distance_to_pivot <= radius)
			{
				min_distance = distance_to_pivot;
				node_id = i;
			}     
		}
		
		return node_id;
	}

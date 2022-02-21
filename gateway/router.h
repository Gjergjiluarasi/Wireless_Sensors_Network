/*
   Wireless Sensor Networks Laboratory

   Technische Universität München
   Lehrstuhl für Kommunikationsnetze
   http://www.lkn.ei.tum.de

   copyright (c) 2017 Chair of Communication Networks, TUM

	project title: Museum Security WSN system

	contributors:
   * Gjergji Luarasi
   * Klea Plaku
   * Jani Mosko
*/

#ifndef ROUTER_H
#define ROUTER_H

#include "core/net/linkaddr.h"

// Standard C includes:
#include <stdint.h>

// the number of nodes present in the network
#ifndef TOTAL_NODES
#define TOTAL_NODES 6
#define INF 16384
#endif

// the number of nodes present in the network
#ifndef TIMER_INTERVAL
#define TIMER_INTERVAL 1.5
#endif

// custom structures
typedef struct
{
	linkaddr_t 	dest[TOTAL_NODES];			// Destination id. Every node should be able to reach every other node plus itself. Thus total entries are equal to total number of nodes.
	linkaddr_t 	next_hop[TOTAL_NODES];		// Next hop in route to destination.
	uint16_t 	cost[TOTAL_NODES]; 			// Number of total hops of the packet route. Maximum 10.
}l_table;

typedef struct{
	linkaddr_t dest;
	char message[32];
	uint8_t hops[TOTAL_NODES-1];
}packet_t;


#endif /* ROUTER_H */

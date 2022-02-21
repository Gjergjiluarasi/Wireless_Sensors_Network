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

/*** INCLUDES ***/

// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h"		// Establish connections.
#include "net/netstack.h"
#include "dev/leds.h"			// Use LEDs.
#include "lib/random.h"

#include "sys/clock.h"			// Use CLOCK_SECOND.
#include "dev/cc2538-rf.h"

// Headers
#include "router.h"
#include "buffer.h"
#include "helpers.h"

// Standard C includes:
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/*** INCLUDES END ***/



/*** FUNCTIONS ***/

//static void print_buffer_contents(void); // Print buffer content function
void BellmanFord(); // Bellman Ford function
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);	// Broadcast receive function
static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from);
static void enqueue_packet(packet_t tx_packet); // Enqueue packet function

/*** FUNCTIONS END ***/


/*** VARIABLES ***/

static uint8_t node_id;// Node id of broadcasting mote

static uint8_t dest_id = 0x09;

//static uint8_t synchronised_nodes[9] = {0, 0, 0, 0, 0, 0, 0, 0 ,0};

//static packet_t routing_packet;

static Buffer buffer;	// the buffer to store packets until they are transmitted

static int cnt = 5;

static int16_t COST_MATRIX[TOTAL_NODES][TOTAL_NODES] = {	// Cost matrix
		{0, INF, INF, INF, INF, INF},
		{INF, 0, INF, INF, INF, INF},
		{INF, INF, 0, INF, INF, INF},
		{INF, INF, INF, 0, INF, INF},
		{INF, INF, INF, INF, 0, INF},
		{INF, INF, INF, INF, INF, 0}
};

static l_table lut[TOTAL_NODES]; /*= {
		{ .cost[0] = 0, .cost[1] = 8192, .cost[2] = 8192, .cost[3] = 8192, .cost[4] = 8192, .cost[5] = 8192},
		{ .cost[0] = 8192, .cost[1] = 0, .cost[2] = 8192, .cost[3] = 8192, .cost[4] = 8192, .cost[5] = 8192},
		{ .cost[0] = 8192, .cost[1] = 8192, .cost[2] = 0, .cost[3] = 8192, .cost[4] = 8192, .cost[5] = 8192},
		{ .cost[0] = 8192, .cost[1] = 8192, .cost[2] = 8192, .cost[3] = 0, .cost[4] = 8192, .cost[5] = 8192},
		{ .cost[0] = 8192, .cost[1] = 8192, .cost[2] = 8192, .cost[3] = 8192, .cost[4] = 0, .cost[5] = 8192},
		{ .cost[0] = 8192, .cost[1] = 8192, .cost[2] = 8192, .cost[3] = 8192, .cost[4] = 8192, .cost[5] = 0},
}; */// Look-up table for the routing


char routing_message[80];	// Routing message - message that forwards the
							// routing array, forming eventually the routing table

/*** VARIABLES END ***/


/*** CONNECTIONS ***/

// Broadcast
static struct broadcast_conn broadcastConn;
static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};

// Unicast
static struct unicast_conn unicast;
static const struct unicast_callbacks unicast_call= {unicast_recv};

/*** CONNECTIONS END ***/


/*** PROCESSES ***/

PROCESS(routing, "Routing process");
PROCESS(receive, "Receiver process");

/*** PROCESSES END ***/


/*** AUTOSTART PROCESSES CALL***/

AUTOSTART_PROCESSES(&routing, &receive);

/*** AUTOSTART PROCESSES CALL END ***/


/*** PROCESS THREADS ***/

PROCESS_THREAD(routing, ev, data) {
	static struct etimer et, broadcast_timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcastConn));
	PROCESS_BEGIN();
	// set your group's channel
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 12);
	//print_settings();
	//check_for_invalid_addr();
	etimer_set(&et, 5*CLOCK_SECOND); //randomize the sending time a little
	while(1){
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		// Reset cost matrix each time
		for(int i = 0; i < TOTAL_NODES; i++)
		{
			for(int j = 0; j < TOTAL_NODES; j++)
						{
							if(i==j)
							{
								COST_MATRIX[i][j] = 0;
							}
							else
							{
								COST_MATRIX[i][j] = INF;
							}
						}
			}

			node_id = (linkaddr_node_addr.u8[1] & 0xFF);

			for(int k = 0; k < TOTAL_NODES; k++)
			{
				if(k != node_id-4)
				{
					lut[node_id-4].cost[k] = INF;
				}
				else
				{
					lut[node_id-4].cost[k] = 0;
				}
				//printf("%d |", lut[node_id-4].cost[k]);
			}

			// Update cost matrix
			etimer_set(&broadcast_timer, CLOCK_SECOND/5 + 0.1*random_rand()/RANDOM_RAND_MAX); //randomize the sending time a little
			unicast_close(&unicast);
			broadcast_open(&broadcastConn,129,&broadcast_callbacks); // The broadcast_recv() is called when the broadcast connection is opened since it is a callback function.

			do
			{
				cnt--;
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&broadcast_timer));
				leds_on(LEDS_RED);
				sprintf(routing_message, "  %d %d %d %d %d %d \n", COST_MATRIX[node_id-4][0], COST_MATRIX[node_id-4][1], COST_MATRIX[node_id-4][2], COST_MATRIX[node_id-4][3], COST_MATRIX[node_id-4][4], COST_MATRIX[node_id-4][5]);
				//printf("%s", routing_message);
		    	packetbuf_copyfrom(routing_message, strlen(routing_message)); // Put on the packet buffer the routing message to forward to other nodes
				broadcast_send(&broadcastConn);
				//printf("Broadcast message sent. cnt =  %d \r\n", cnt);
				etimer_reset(&broadcast_timer);
				leds_off(LEDS_RED);
			}	while(cnt >= 0);
			cnt = 5;
			//enqueue_packet(tx_packet);
			broadcast_close(&broadcastConn);
			BellmanFord();
			//print_lookup_table(lut[node_id - 1], TOTAL_NODES);
			//tx_packet.dest = dest_id;
			//tx_packet.message = led_color;
			//tx_packet.hops[node_id-4] = 1;
			//enqueue_packet(tx_packet);
			etimer_reset(&et);
			process_post(&receive, PROCESS_EVENT_MSG, 0);
		}
		PROCESS_END();
}

// Process: receive packet
PROCESS_THREAD(receive, ev, data) {
	PROCESS_EXITHANDLER(unicast_close(&unicast));
	PROCESS_BEGIN();
	//NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 12);
	//print_settings();

    unicast_open(&unicast, 129, &unicast_call);
	// If all is OK, we can start the other two processes:

	while(1) {
		// Contiki processes cannot start loops that never end.
		PROCESS_WAIT_EVENT();


	}
	PROCESS_END();
}

/*** PROCESS THREADS END ***/

//---------------- FUNCTION DEFINITIONS ----------------

/*** FUNCTIONS ***/

// Defines the behavior of a broadcast connection upon receiving data.
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{

	char rxBuffer[PACKETBUF_SIZE];
	static int rxBytes = 0;
	leds_on(LEDS_GREEN);
	node_id = (linkaddr_node_addr.u8[1] & 0xFF);

	/* Get packet length */
	uint8_t len = strlen( (char *)packetbuf_dataptr() );
	/* Get packet's RSSI */
	int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	// Update the row of the cost matrix coming from the sender mote's routing message
	rxBytes = packetbuf_copyto(&rxBuffer);
	if(rxBytes > 0)
	{
		char *splt = strtok(rxBuffer, " ");
		int i = 0;
		while( splt != NULL ) {
			if(atoi(splt)!=0)
				COST_MATRIX[from->u8[1]-4][i] = atoi(splt);
			i++;
			splt = strtok(NULL, " ");
		   }
	}
	// Update the cost matrix entry of the connection between sender mote and own receiver mote
	COST_MATRIX[node_id-4][from->u8[1]-4] = rssi*rssi;
	/* Print address of sending node */
	printf("Got RX packet (broadcast)to 0x%x (self) from: 0x%x%x, len: %d, RSSI: %d = -(%d).\n", node_id,from->u8[0], from->u8[1], len, rssi, COST_MATRIX[node_id-4][from->u8[1]-4]);
	/*printf("COST_MATRIX:\n");
	for(int i = 0; i < TOTAL_NODES; ++i)
	{
		for(int j = 0; j < TOTAL_NODES; ++j)
		{
			printf("%d ", COST_MATRIX[i][j]);
		}
		printf("\n");
	}
	printf("Routing message : %s\n Packet received: ", routing_message);*/
	//print_buffer_contents();
	leds_off(LEDS_GREEN);
}

// Print the received message
/*static void print_buffer_contents(void){
	packet_t rx_packet;
	packetbuf_copyto(&rx_packet);
	//printf("[Source 0x%x%x, broadcast, RSSI %d] Message is:\t", r->u8[0], from->u8[1], (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
	printf("Received buffered message: %s", rx_packet.message);
}*/

// Defines the behavior of a connection upon receiving data.
static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
	packet_t rx_packet;
	packetbuf_copyto(&rx_packet);
	leds_on(LEDS_GREEN);
	printf("[Source 0x%x%x, broadcast, RSSI %d] Message is:\t", from->u8[0], from->u8[1], (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
	printf("%s", rx_packet.message);
	leds_off(LEDS_GREEN);
}

// Enqueue packet function: sets a timer and puts this timer and the packet in the buffer
static void enqueue_packet(packet_t tx_packet){
	uint8_t return_code;
	struct timer packet_timer;

	timer_set(&packet_timer, CLOCK_SECOND * TIMER_INTERVAL);

	// put packet and timer in the queue
	return_code = BufferIn(&buffer, tx_packet, packet_timer);

	// check if storing was successful
	if (return_code == BUFFER_FAIL) {
		printf("The buffer is full. Consider increasing the 'BUFFER_SIZE' value"
				" in 'buffer.h'\n");
	} else {
		// inform the send process a new packet was enqueued
		process_post(&receive, PROCESS_EVENT_MSG, 0);
	}
}

// Bellman Ford function
void BellmanFord(){
	// Create routing table array of node structures
	struct node
	{
	    uint16_t dist[TOTAL_NODES];
	    uint8_t from[TOTAL_NODES];
	}rt[TOTAL_NODES];

	for( int i = 0; i < TOTAL_NODES; i++)
	{
		for(int k = 0; k < TOTAL_NODES; k++)
		{
			if( i!=k )
			{
				rt[i].dist[k] = 8192;
			}
			else
			{
				rt[i].dist[k] = 0;
			}
			//printf("%d |", rt[i].dist[k]);
		}

	}

	// Transpose the COST_MATRIX to use it in Bellman Ford algorithm
	int16_t costmat[TOTAL_NODES][TOTAL_NODES];
	uint8_t count = 0,i,j,k;
	for( i = 0; i<TOTAL_NODES; i++)
	{
		for( j = 0; j < TOTAL_NODES; j++)
	    {
			costmat[j][i] = COST_MATRIX[i][j];
	    }
	}

	for( i=0;i<TOTAL_NODES;i++)
	{
		for( j=0;j<TOTAL_NODES;j++)
		{
			rt[i].dist[j]=costmat[i][j];	// initialize the distance equal to cost matrix
	        rt[i].from[j]=j;
	    }
	}

	do {
		count=0;
		for(i=0;i<TOTAL_NODES;i++)	// We choose arbitrary vertex k and we calculate the direct distance from the node i to k using the cost matrix
	    //and add the distance from k to node j
			for(j=0;j<TOTAL_NODES;j++)
				for(k=0;k<TOTAL_NODES;k++)
					if(rt[i].dist[j]>costmat[i][k]+rt[k].dist[j])	// We calculate the minimum distance
					{
	                    rt[i].dist[j]=rt[i].dist[k]+rt[k].dist[j];
	                    rt[i].from[j]=k;
	                    count++;
	                }
	}while(count!=0);

	for(i=0;i<TOTAL_NODES;i++)
	{
		printf("\n\n For router %d\n",i+1);
		for(j=0;j<TOTAL_NODES;j++)
		{
			lut[i].dest[j].u8[1] = j+1;
	        lut[i].next_hop[j].u8[1] = rt[i].from[j]+1;
	        lut[i].cost[j] = rt[i].dist[j];
	        printf("\t\ndest node %d next hop: %d Distance %d ",lut[i].dest[j].u8[1], lut[i].next_hop[j].u8[1], lut[i].cost[j]);
	     }
	}
}

// Prints the current settings.
/*void print_settings(void){
	radio_value_t channel;

	NETSTACK_CONF_RADIO.get_value(RADIO_PARAM_CHANNEL,&channel);

	printf("\n-------------------------------------\n");
	printf("RIME addr = \t0x%x%x\n",
			linkaddr_node_addr.u8[0],
			linkaddr_node_addr.u8[1]);
	printf("Using radio channel %d\n", channel);
	printf("---------------------------------------\n");
}*/

/*** FUNCTIONS END ***/


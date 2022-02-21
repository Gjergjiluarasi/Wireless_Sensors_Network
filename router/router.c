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

// -----------------------------------------------------------------------------

/*** INCLUDES ***/

// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h"	// Establish connections.
#include "net/netstack.h"
#include "lib/random.h"
#include "dev/leds.h"

// Standard C includes:
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Headers
#include "router.h"
#include "buffer.h"
#include "helpers.h"

/*** INCLUDES END ***/

// -----------------------------------------------------------------------------

/*** FUNCTIONS ***/

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);	// Broadcast receive function

//static void print_buffer_contents(void); // Print buffer content function

void BellmanFord(); // Bellman Ford function

static void send_packet(packet_t tx_packet); // Send packet function

static void enqueue_packet(packet_t tx_packet); // Enqueue packet function

static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from);	// Unicast receive function

/*** FUNCTIONS END ***/

// -----------------------------------------------------------------------------

/*** VARIABLES ***/

static uint8_t node_id;// Node id of broadcasting mote

//static uint8_t dest_id = 0x09;

//static uint8_t synchronised_nodes[9] = {0, 0, 0, 0, 0, 0, 0, 0 , 0};

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

// -----------------------------------------------------------------------------

/*** CONNECTIONS ***/
static struct broadcast_conn broadcastConn;	// Broadcast connection

static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};	// Callback functions to the broadcast

static struct unicast_conn unicast; // Creates an instance of a unicast

static const struct unicast_callbacks unicast_call = {unicast_recv};	// Callback functions to the unicast


/*** CONNECTIONS END ***/

// -----------------------------------------------------------------------------

/*** PROCESSES ***/
//PROCESS(synchronization_process, "Synchronizes the motes and signals the signaling process to start");
PROCESS(signaling_process, "Broadcasts signaling messages and fills the cost matrix for network topology");
PROCESS(send_process, "Process to send packets");

/*** PROCESSES END ***/

// -----------------------------------------------------------------------------

/*** AUTOSTART PROCESSES CALL***/

AUTOSTART_PROCESSES(&signaling_process ,&send_process);

/*** AUTOSTART PROCESSES CALL END ***/

// -----------------------------------------------------------------------------


/*** PROCESS THREADS ***/

// Process: signaling for cost matrix creation
PROCESS_THREAD(signaling_process, ev, data) {

	static struct etimer et, broadcast_timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcastConn));
	PROCESS_BEGIN();
	// set your group's channel
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 12);

	print_settings();
	check_for_invalid_addr();

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
		process_post(&send_process, PROCESS_EVENT_MSG, 0);
	}
	PROCESS_END();
}

// Process: send packet
PROCESS_THREAD(send_process, ev, data) {
	PROCESS_EXITHANDLER(unicast_close(&unicast));
	PROCESS_BEGIN();
	printf("send_process: started\n");

	static struct etimer t;
	static struct timer packet_timer;
	static packet_t tx_packet;
	static uint8_t return_code;
	static uint8_t is_processing_packet = 0;
	static clock_time_t remaining_time;

	while(1) {
		PROCESS_WAIT_EVENT();

		// a new packet has been added to the buffer
		if (ev == PROCESS_EVENT_MSG)
		{

			// is a packet already processed?
			if ( ! is_processing_packet )
			{
				is_processing_packet = 1;
				// get a packet from the buffer
				return_code = BufferOut(&buffer, &tx_packet, &packet_timer);
				// there was nothing in the buffer
				if (return_code == BUFFER_FAIL)
				{
					is_processing_packet = 0;
				}
				// there was something in the buffer
				else
				{
					remaining_time = timer_remaining(&packet_timer);
					// check if the timer has already expired
					if ( timer_expired(&packet_timer) )
					{
						broadcast_close(&broadcastConn);
						unicast_open(&unicast, 129, &unicast_call);

						send_packet(tx_packet);
			    		unicast_close(&unicast);

						is_processing_packet = 0;
						process_post(&send_process, PROCESS_EVENT_MSG, 0);
					}
					else
					{
						// wait for the remaining time
						etimer_set(&t, remaining_time);
					}
				}
			}
		}
		else if (ev == PROCESS_EVENT_TIMER)
		{
			// timer indicating time to send expired
			if (etimer_expired(&t))
			{
				send_packet(tx_packet);
				is_processing_packet = 0;
				// tell the process to check if there is another packet in the
				// buffer
				process_post(&send_process, PROCESS_EVENT_MSG, 0);
			}
		}
	}
	PROCESS_END();
}

/*** PROCESS THREADS END ***/

// -----------------------------------------------------------------------------

/*** FUNCTIONS ***/

// Broadcast receive function
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


// Print buffer content function
/*static void print_buffer_contents(void){
	char rxBuffer[PACKETBUF_SIZE];
	int rxBytes = 0;

	rxBytes = packetbuf_copyto(&rxBuffer);

	if (rxBytes>0) {
		printf("%s\r\n",rxBuffer);
	}
}*/

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

// Send packet function
static void send_packet(packet_t tx_packet){
	uint8_t i;
	// Define next hop and forward packet

	for(i = 0; i < TOTAL_NODES; i++)
	{
		if(linkaddr_cmp(&tx_packet.dest, &lut[node_id - 1].dest[i]))
		{
			leds_on(LEDS_YELLOW);
			packetbuf_copyfrom(&tx_packet, sizeof(packet_t));
			unicast_send(&unicast, &lut[node_id - 1].next_hop[i]);
			leds_off(LEDS_YELLOW);
			break;
		}
	}
	//turn_off(tx_packet.message);
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
		process_post(&send_process, PROCESS_EVENT_MSG, 0);
	}
}

// Unicast receive function: defines the behavior of a connection upon receiving data
static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
	packet_t rx_packet;
	packet_t tx_packet;

	packetbuf_copyto(&rx_packet);


		tx_packet.dest.u8[0] = rx_packet.dest.u8[0];
		tx_packet.dest.u8[1] = rx_packet.dest.u8[1];
		//rx_packet.hops[node_id - 4] = 1;

		for(int i = 0; i< TOTAL_NODES-1; i++)
		{
			tx_packet.hops[i] = rx_packet.hops[i];
		}
		tx_packet.hops[node_id - 4] = 1;


		strcpy(tx_packet.message, rx_packet.message);


	//turn_on(rx_packet.message);
	enqueue_packet(tx_packet);
}

/*** FUNCTIONS END ***/

//make TARGET=zoul BOARD=remote-revb PORT=/dev/ttyUSB0 login


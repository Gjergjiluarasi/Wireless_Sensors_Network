
// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h"     // Establish connections.
#include "net/netstack.h"      // Wireless-stack definitions
#include "lib/random.h"
#include "dev/leds.h"          // Use LEDs.

#include "dev/adc-zoul.h"      // ADC
#include "dev/zoul-sensors.h"  // Sensor functions
#include "dev/sys-ctrl.h"
#include "sys/clock.h"			// Use CLOCK_SECOND.
//#include "dev/cc2538-rf.h"

#include "core/net/linkaddr.h"


// Standard C includes:
#include <stdio.h>      // For printf.
#include <stdint.h>
#include <string.h> // for itoa(sensor.value)
#include <stdlib.h>

// Headers
#include "helpers.h"

// Reading frequency in seconds.
#define TEMP_READ_INTERVAL CLOCK_SECOND*1
#define NEXT_NODES 6
#define INF 25000


/*** VARIABLES ***/

static	linkaddr_t  nextHop;
static int dest_id = 9;
typedef struct{
	linkaddr_t dest;
	char message[32];
	uint8_t hops[NEXT_NODES-1];
}packet_t;
static int16_t COST_VECTOR[NEXT_NODES] = {INF, INF, INF, INF, INF, INF};
static int cnt = 3;

/*** VARIABLES END ***/


/*** FUNCTIONS ***/

static int getDistanceSensorValue(uint16_t adc_input);
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);
int find_min_arg();

/*** FUNCTIONS END ***/



/*** CONNECTIONS ***/

static struct broadcast_conn broadcastConn;
static struct unicast_conn unicast;
static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};
static const struct unicast_callbacks unicast_call = {};

/*** CONNECTIONS END ***/


/*** PROCESSES DECLARATION ***/

PROCESS (routing, "Sensor Mote Routing process");
PROCESS (send, "Sensor Mote packet sending process");

/*** PROCESSES DECLARATION END ***/



AUTOSTART_PROCESSES (&routing, &send);



/*** ROUTING PROCESS CODE ***/
PROCESS_THREAD(routing, ev, data) {

	static struct etimer s_reading_timer, broadcast_timer;
	PROCESS_EXITHANDLER(broadcast_close(&broadcastConn));
	PROCESS_BEGIN();
	// set your group's channel
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 12);
	print_settings();
	broadcast_open(&broadcastConn, 129, &broadcast_callbacks);
	etimer_set(&s_reading_timer, 5*CLOCK_SECOND);
	while(1)
	{

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&s_reading_timer));
		for(int j = 0; j < NEXT_NODES; j++)
		{
			COST_VECTOR[j] = INF;
		}
		etimer_set(&broadcast_timer, CLOCK_SECOND/5 + 0.1*random_rand()/RANDOM_RAND_MAX); //randomize the sending time a little
		unicast_close(&unicast);
		broadcast_open(&broadcastConn,129,&broadcast_callbacks); // The broadcast_recv() is called when the broadcast connection is opened since it is a callback function.
		do
				{
					cnt--;
					PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&broadcast_timer));
					leds_on(LEDS_BLUE);
					packetbuf_copyfrom("H", strlen("H")); // Put on the packet buffer the routing message to forward to other nodes
					broadcast_send(&broadcastConn);
					printf("Broadcast message sent. cnt =  %d \r\n", cnt);
					etimer_reset(&broadcast_timer);
					leds_off(LEDS_BLUE);
		}	while(cnt >= 0);
		cnt = 3;
		broadcast_close(&broadcastConn);
		etimer_reset(&s_reading_timer);
		process_post(&send, PROCESS_EVENT_MSG, 0);
	}
	PROCESS_END();
}
/*** ROUTING PROCESS CODE END ***/


/*** SEND PROCESS CODE  ***/
PROCESS_THREAD (send, ev, data) {

	PROCESS_EXITHANDLER(unicast_close(&unicast));
	PROCESS_BEGIN ();
	// Timer
		static struct etimer sensor_reading_timer;
		// Value of ADC coming from the Distance sensor
		static uint16_t adc3_value;
		static int distance_read = 100;
		static uint8_t stat=0;
		packet_t tx_packet;
	// Set Group's channel ( 10 + 2  for Group 2)
	//NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,12);
	// Configure ADC
	adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC3);
	// Set measurement timer
	etimer_set(&sensor_reading_timer, TEMP_READ_INTERVAL);
	// Open unicast connection
	while (1)
	{
		PROCESS_WAIT_EVENT();
		// a new packet has been added to the buffer
		if (ev == PROCESS_EVENT_MSG)
		{
		// Open unicast connection.
			//unicast_open(&unicast, 129, &unicast_call);

	    if(ev == PROCESS_EVENT_TIMER)
	    {

	    	dest_id = 9;

	    	leds_on(LEDS_PURPLE);
    		adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;
	    	distance_read = getDistanceSensorValue(adc3_value);

	    	if (distance_read <= 20  && distance_read >= 0 )
	    	{
	    		stat = 1;
	    		// Add buzzer
	    	}
	    	else
	    	{
	    		stat = 0;
	    	}

	    	sprintf(tx_packet.message, "RightSensor %d %d %d\n", vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED), cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED), stat);
	    	tx_packet.dest.u8[0] = (dest_id >> 8) & 0xFF;
	    	tx_packet.dest.u8[1] = dest_id & 0xFF;
	    	for (int i = 0; i< NEXT_NODES-1; i++)
	    	{
	    		tx_packet.hops[i] = 0;
	    	}
	    	// Create link address of next hop ( which is the closest router )
	    	nextHop.u8[0] = ((find_min_arg()+4) >> 8) & 0xFF;
	    	nextHop.u8[1] = (find_min_arg()+4) & 0xFF;
	    	// Copy tx packet to sending buffer

	    	broadcast_close(&broadcastConn);
			unicast_open(&unicast, 129, &unicast_call);


	    	packetbuf_copyfrom(&tx_packet, sizeof(packet_t));
	    	// Send packet
	    	unicast_send(&unicast, &nextHop);
	    	//packetbuf_copyfrom(message, strlen(message));
    		leds_off(LEDS_PURPLE);
    		unicast_close(&unicast);

    		etimer_reset(&sensor_reading_timer);
	    }
		}
    }
	PROCESS_END ();
}
/*** SEND PROCESS CODE END ***/


// Function that translates the ADC value to distance value in cm
static int getDistanceSensorValue(uint16_t adc_input)
{
	float distance = 4800/(adc_input-20);
	return distance;
}

// Broadcast receive function
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	leds_on(LEDS_GREEN);
	uint8_t len = strlen( (char *)packetbuf_dataptr() );
	int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	if (from->u8[1] >= 4) //check if sensor or router
	{
		COST_VECTOR[from->u8[1]-4] = rssi*rssi;
	}
	//printf("Got RX packet (broadcast) from: 0x%x%x, len: %d, RSSI: %d. Cost Vector:\r\n",from->u8[0], from->u8[1],len,rssi);
	/*for(int j = 0; j < NEXT_NODES; j++)
	{
		printf("%d ", COST_VECTOR[j]);
	}*/
	//printf("\nMin arg of cost vector: %d\n", find_min_arg());
	leds_off(LEDS_GREEN);
}

// Finds argument of min cost wrt router
int find_min_arg()
{
	uint16_t min = 20000;
	int argmin = 10;
	for(int i = 0; i < NEXT_NODES; i++)
	{
		if(COST_VECTOR[i]<min)
		{
			min = COST_VECTOR[i];
			argmin = i;
		}
	}
	return argmin;
}

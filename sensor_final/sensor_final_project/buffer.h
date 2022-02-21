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


#ifndef BUFFER_H
#define BUFFER_H

#include "contiki.h"
#include "router.h"

#include <stdint.h>

// maximum size of the buffer
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 10
#endif

// return code for buffer failure
#ifndef BUFFER_FAIL
#define BUFFER_FAIL 0
#endif

// return code for buffer success
#ifndef BUFFER_SUCCESS
#define BUFFER_SUCCESS  1
#endif

// buffer structure
typedef struct
{
	struct timer timers[BUFFER_SIZE];
	packet_t packets[BUFFER_SIZE];
	uint8_t read;
	uint8_t write;
}Buffer;

// puts a packet and a timer in the buffer
// returns BUFFER_FAIL if buffer is full
uint8_t BufferIn(Buffer *buffer, packet_t packet, struct timer packet_timer);

// removes a packet from a buffer
// returns BUFFER_FAIL if buffer is empty
uint8_t BufferOut(Buffer *buffer, packet_t *packet, struct timer *packet_timer);

#endif /* BUFFER_H */

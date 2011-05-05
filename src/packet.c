#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "s710.h"

/* defines the packet types */

static packet_t gPacket[] = {

	/* packet name, subtype, payload length, checksum value, payload data */
	/* note that checksum values are calculated at packet assembly time.  */
  
	/* S710_GET_OVERVIEW */

	{ "get overview",      S710_REQUEST, 0x15, 0, 0x3790, { 0 } },

	/* S710_GET_USER */

	{ "get user",          S710_REQUEST, 0x06, 0, 0x4b96, { 0 } },

	/* S710_GET_WATCH */

	{ "get watch",         S710_REQUEST, 0x02, 0, 0x9b95, { 0 } },

	/* S710_GET_LOGO */

	{ "get logo",          S710_REQUEST, 0x10, 0, 0x7390, { 0 } },

	/* S710_GET_BIKE */

	{ "get bike",          S710_REQUEST, 0x14, 0, 0xa393, { 0 } },

	/* S710_GET_EXERCISE_1 */

	{ "get exercise 1",    S710_REQUEST, 0x04, 1, 0x1d2f, { 0x11 } },

	/* S710_GET_EXERCISE_2 */

	{ "get exercise 2",    S710_REQUEST, 0x04, 1, 0x1d85, { 0x22 } },

	/* S710_GET_EXERCISE_3 */

	{ "get exercise 3",    S710_REQUEST, 0x04, 1, 0x1de3, { 0x33 } },

	/* S710_GET_EXERCISE_4 */

	{ "get exercise 4",    S710_REQUEST, 0x04, 1, 0x1cd1, { 0x44 } },

	/* S710_GET_EXERCISE_5 */

	{ "get exercise 5",    S710_REQUEST, 0x04, 1, 0x1cb7, { 0x55 } },

	/* S710_GET_REMINDER_1 */

	{ "get reminder 1",    S710_REQUEST, 0x0e, 1, 0x1e79, { 0x00 } },

	/* S710_GET_REMINDER_2 */

	{ "get reminder 2",    S710_REQUEST, 0x0e, 1, 0x9e7c, { 0x01 } },

	/* S710_GET_REMINDER_3 */

	{ "get reminder 3",    S710_REQUEST, 0x0e, 1, 0x9e76, { 0x02 } },

	/* S710_GET_REMINDER_4 */

	{ "get reminder 4",    S710_REQUEST, 0x0e, 1, 0x1e73, { 0x03 } },

	/* S710_GET_REMINDER_5 */

	{ "get reminder 5",    S710_REQUEST, 0x0e, 1, 0x9e62, { 0x04 } },

	/* S710_GET_REMINDER_6 */

	{ "get reminder 6",    S710_REQUEST, 0x0e, 1, 0x1e67, { 0x05 } },

	/* S710_GET_REMINDER_7 */

	{ "get reminder 7",    S710_REQUEST, 0x0e, 1, 0x1e6d, { 0x06 } },

	/* S710_GET_FILES */

	{ "get files",         S710_REQUEST, 0x0b, 0, 0x2f95, { 0 } },   

	/* S710_CONTINUE_TRANSFER */

	{ "continue transfer", S710_REQUEST, 0x16, 0, 0x0b90, { 0 } },

	/* S710_CLOSE_CONNECTION */

	{ "close connection",  S710_REQUEST, 0x0a, 0, 0xbb96, { 0 } },

	/* S710_SET_USER */

	{ "set user",          S710_REQUEST, 0x05, 21, 0, { 0 } },

	/* S710_SET_WATCH */

	{ "set watch",         S710_REQUEST, 0x01, 11, 0, { 0 } },

	/* S710_SET_LOGO */

	{ "set logo",          S710_REQUEST, 0x0f, 47, 0, { 0 } },

	/* S710_SET_BIKE */

	{ "set bike",          S710_REQUEST, 0x13, 25, 0, { 0 } },

	/* S710_SET_EXERCISE_1 */

	{ "set exercise 1",    S710_REQUEST, 0x03, 23, 0, { 0x11 } },

	/* S710_SET_EXERCISE_2 */

	{ "set exercise 2",    S710_REQUEST, 0x03, 23, 0, { 0x22 } },

	/* S710_SET_EXERCISE_3 */

	{ "set exercise 3",    S710_REQUEST, 0x03, 23, 0, { 0x33 } },

	/* S710_SET_EXERCISE_4 */

	{ "set exercise 4",    S710_REQUEST, 0x03, 23, 0, { 0x44 } },

	/* S710_SET_EXERCISE_5 */

	{ "set exercise 5",    S710_REQUEST, 0x03, 23, 0, { 0x55 } },

	/* S710_SET_REMINDER_1 */

	{ "set reminder 1",    S710_REQUEST, 0x0d, 14, 0, { 0x00 } },

	/* S710_SET_REMINDER_2 */

	{ "set reminder 2",    S710_REQUEST, 0x0d, 14, 0, { 0x01 } },

	/* S710_SET_REMINDER_3 */

	{ "set reminder 3",    S710_REQUEST, 0x0d, 14, 0, { 0x02 } },

	/* S710_SET_REMINDER_4 */

	{ "set reminder 4",    S710_REQUEST, 0x0d, 14, 0, { 0x03 } },

	/* S710_SET_REMINDER_5 */

	{ "set reminder 5",    S710_REQUEST, 0x0d, 14, 0, { 0x04 } },

	/* S710_SET_REMINDER_6 */

	{ "set reminder 6",    S710_REQUEST, 0x0d, 14, 0, { 0x05 } },

	/* S710_SET_REMINDER_7 */

	{ "set reminder 7",    S710_REQUEST, 0x0d, 14, 0, { 0x06 } },

	/* S710_HARD_RESET */

	{ "hard reset",        S710_REQUEST, 0x09, 0, 0x8796, { 0 } }
};

static int gNumPackets = sizeof(gPacket)/sizeof(gPacket[0]);

/* return a packet pointer for a given packet index */

packet_t *
packet_get ( S710_Packet_Index idx )
{
	return ( idx > S710_PACKET_INDEX_INVALID && idx < gNumPackets ) ? &gPacket[idx] : NULL;
}


/* get a single-packet response to a request */

packet_t *
packet_get_response(S710_Packet_Index request, struct s710_driver *d)
{
	packet_t *send;
	packet_t *recv = NULL;

	send = packet_get(request);

	if ( send != NULL && packet_send(send,d) > 0 )
		recv = packet_recv(d);
	if ( recv == NULL ) {
		if ( send != NULL ) {
			if ( request != S710_CONTINUE_TRANSFER &&
				 request != S710_CLOSE_CONNECTION ) {
				fprintf(stderr,"\n%s: %s\n\n",send->name,
						(errno) ? strerror(errno) : "No response");
			}
		} else {
			fprintf(stderr,"\n%d: bad packet index\n\n",request);
		}
	}

	return recv;
}

void
packet_print(packet_t *p, FILE *fp)
{
	int i;

	if ( !p ) {
		fprintf(fp,"\nNULL packet received in print_packet!\n");
		return;
	}

	fprintf(fp,"\nPacket ID:       0x%02x\n",p->id);
	fprintf(fp,"Packet size:     %d bytes\n",p->length);
	if ( p->length ) {
		fprintf(fp,"Packet data:\n\n");
		for ( i = 0; i < p->length; i++ ) {
			fprintf(fp,"%02x ",p->data[i]);
			if ( i % 16 == 15 ) fprintf(fp,"\n");
		}
		fprintf(fp,"\n\n");
	}

	fprintf(fp,"Packet checksum: 0x%04x\n",p->checksum);
	fprintf(fp,"\n");
}

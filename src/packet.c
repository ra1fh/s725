/*
 * sending/receiving of packets
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "packet.h"
#include "utils.h"

/* defines the packet types */

typedef const char    *PacketName;
typedef unsigned char  PacketType;
typedef unsigned char  PacketID;
typedef unsigned short PacketLength;
typedef unsigned short PacketChecksum;
typedef unsigned char  PacketData[1];

struct packet {
	PacketName     name;
	PacketType     type;
	PacketID       id;
	PacketLength   length;
	PacketChecksum checksum;
	PacketData     data;
};

unsigned char*
packet_data(packet_t *p)
{
	return p->data;
}

unsigned short
packet_len(packet_t *p)
{
	return p->length;
}

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
packet_get_response(S710_Packet_Index request)
{
	packet_t *send;
	packet_t *recv = NULL;

	send = packet_get(request);

	if (send != NULL && packet_send(send) > 0)
		recv = packet_recv();
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

/* This file needs to be rewritten.  */
#define READ_TRIES   10

/* static helper functions */
static int packet_recv_short(unsigned short *s);
static unsigned short packet_checksum(packet_t *p);
static int packet_serialize(packet_t *p, BUF *buf);

/* 
 * send a packet via the S710 driver
 */
int 
packet_send(packet_t *p)
{
	int  ret = 1;
	BUF *buf;

	buf = buf_alloc(0);
	p->checksum = packet_checksum(p);
  
	packet_serialize(p, buf);

	ret = driver_write(buf);

	buf_free(buf);

	return ret != -1;
}

/*
 * receive a packet from the S710 driver (allocates memory)
 */
packet_t *
packet_recv() {
	int             r;
	int             i;
	unsigned char   c = 0;
	unsigned char   id;
	unsigned short  len;
	size_t          siz;
	packet_t       *p = NULL;
	unsigned short  crc = 0;

	r = driver_read_byte(&c);
	crc_process ( &crc, c );

	if ( c == S710_RESPONSE ) {
		r = driver_read_byte(&id);
		crc_process ( &crc, id );
		r = driver_read_byte(&c);
		crc_process ( &crc, c );
		r = packet_recv_short (&len);
		crc_process ( &crc, len >> 8 );
		crc_process ( &crc, len & 0xff );
		if ( r ) {
			len -= 5;
			siz = (len <= 1) ? 0 : len - 1;
			p = calloc(1,sizeof(packet_t) + siz);

			if ( !p ) {
				fprintf(stderr,"calloc(1,%ld): %s\n",
						(long)(sizeof(packet_t) + siz), strerror(errno));
			} else {
				p->type   = S710_RESPONSE;
				p->id     = id;
				p->length = len;
				for ( i = 0; i < len; i++ ) {
					r = driver_read_byte(&p->data[i]);
					crc_process ( &crc, p->data[i] );
					if ( !r ) {
						fprintf(stderr, "driver_read_byte failed\n");
						free ( p );
						p = NULL;
						break;
					}
				}
				if ( p != NULL ) {
					packet_recv_short (&p->checksum);

					if ( crc != p->checksum ) {
	    
						/* 
						   if the checksum failed, we have to jettison the whole 
						   transmission.  i don't yet know how to request a single
						   packet to be resent by the watch.  all we can do is 
						   cancel the download and have the user attempt it again.
						*/
	    
						fprintf ( stderr, 
								  "\nCRC failed [id %d, length %d]\n", 
								  p->id, p->length );
						free ( p );
						p = NULL;
					}
				}
			}
		}
	}
	return p;
}

/*
 * read a short from fd
 */
static int
packet_recv_short(unsigned short *s)
{
	int           r = 0;
	unsigned char u = 0;
	unsigned char l = 0;

	r = driver_read_byte(&u);
	r = driver_read_byte(&l);

	*s = (unsigned short)(u<<8)|l;

	return r;
}

static unsigned short
packet_checksum(packet_t *p)
{
	unsigned short crc = 0;

	crc_process ( &crc, S710_REQUEST );
	crc_process ( &crc, p->id );
	crc_process ( &crc, 0 );
	crc_process ( &crc, ( p->length + 5 ) >> 8 );
	crc_process ( &crc, ( p->length + 5 ) & 0xff );
	crc_block ( &crc, p->data, p->length );
	return crc;
}

static int
packet_serialize(packet_t *p, BUF *buf)
{
	unsigned short l = p->length + 5;

	buf_putc(buf, p->type);
	buf_putc(buf, p->id);
	buf_putc(buf, 0);
	buf_putc(buf, l >> 8);
	buf_putc(buf, l & 0xff);
	if ( p->length > 0 ) {
		buf_append(buf, p->data, p->length);
	}
	buf_putc(buf, p->checksum >> 8);
	buf_putc(buf, p->checksum & 0xff);

	return buf_len(buf);
}

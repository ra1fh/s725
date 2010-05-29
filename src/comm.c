/* $Id: comm.c,v 1.10 2004/09/21 08:16:05 dave Exp $ */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/time.h>
#include "s710.h"

/* This file needs to be rewritten.  */

#define READ_TRIES   10

/* static helper functions */

static void send_byte         ( unsigned char   byte,  S710_Driver *d );
static int  recv_byte         ( unsigned char  *byte,  S710_Driver *d );
static int  recv_short        ( unsigned short *s,     S710_Driver *d );
static unsigned short packet_checksum ( packet_t *p );
static int  serialize_packet  ( packet_t *p, unsigned char *buf );
static void hexdump           (void *ptr, int buflen);

/* send a packet via the S710 driver */

int 
send_packet ( packet_t *p, S710_Driver *d )
{
	int           i;
	int           ret = 1;
	unsigned char serialized[BUFSIZ];
	int           bytes;

	/* first, compute the packet checksum */
  
	p->checksum = packet_checksum(p);
  
	/* next, serialize the packet into a stream of bytes */

	bytes = serialize_packet ( p, serialized );
  

	if ( d->type == S710_DRIVER_USB ) {
    
		/* USB packets are sent all at once. */

		ret = send_packet_usb ( serialized, bytes, d );

	} else {
		for ( i = 0; i < bytes; i++ ) {
			send_byte(serialized[i],d);
#ifdef S710_SERIAL_ALT_INTER_CHAR_TIMER_IMP
			usleep(10000); /* 10ms */
#endif
		}
    
		if ( d->type == S710_DRIVER_SERIAL ) {

			/* the data that gets echoed back is not RS-232 data.  it is garbage 
			   data that we have to flush.  there is a pause of at least 0.1 
			   seconds before the real data shows up. */

			usleep(100000);
			tcflush((int)d->data,TCIFLUSH);
		}
	}

	return ret;
}


/* receive a packet from the S710 driver (allocates memory) */

packet_t *
recv_packet ( S710_Driver *d ) {
	int             r;
	int             i;
	unsigned char   c = 0;
	unsigned char   id;
	unsigned short  len;
	size_t          siz;
	packet_t       *p = NULL;
	unsigned short  crc = 0;

	r = recv_byte ( &c, d );
	crc_process ( &crc, c );

	fprintf(stderr, "recv_byte: com %hhx\n", c);
	if ( c == S710_RESPONSE ) {
		r = recv_byte ( &id, d );
		fprintf(stderr, "recv_byte: id  %hhx\n", id);
		crc_process ( &crc, id );
		r = recv_byte ( &c, d );
		fprintf(stderr, "recv_byte: ??  %hhx\n", id);
		crc_process ( &crc, c );
		r = recv_short ( &len, d );
		fprintf(stderr, "recv_byte: len %hx\n", len);
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
					r = recv_byte ( &p->data[i], d );
					crc_process ( &crc, p->data[i] );
					if ( !r ) {
						fprintf(stderr, "recv_byte failed\n");
						free ( p );
						p = NULL;
						break;
					}
				}
				if ( p != NULL ) {
					recv_short ( &p->checksum, d );

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
					} else {
						hexdump(p->data, p->length);
					}
				}
			}
		}
	}
	return p;
}


/* read a byte from fd */

static int
recv_byte ( unsigned char *byte, S710_Driver *d )
{
	int r = 0;

	switch ( d->type ) {
	case S710_DRIVER_SERIAL:
	case S710_DRIVER_IR:
		r = read_serial_byte(d,byte);
		break;
	case S710_DRIVER_USB:
		r = read_usb_byte(d,byte);
		break;
	default:
		break;
	}

	return r;
}


/* read a short from fd */

static int
recv_short ( unsigned short *s, S710_Driver *d )
{
	int           r = 0;
	unsigned char u = 0;
	unsigned char l = 0;

	r = recv_byte ( &u, d );
	r = recv_byte ( &l, d );

	*s = (unsigned short)(u<<8)|l;

	return r;
}


/* send a mapped byte over fd - MAKE SURE you compute_byte_map() first
   if you are using the serial port IR dongle! */

static void
send_byte ( unsigned char byte, S710_Driver *d )
{
	switch ( d->type ) {
	case S710_DRIVER_SERIAL:
		write((int)d->data,&gByteMap[byte],1);
		break;
	case S710_DRIVER_IR:
		write((int)d->data,&byte,1);
		break;
	default:
		break;
	}
}


static unsigned short
packet_checksum ( packet_t *p )
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
serialize_packet ( packet_t *p, unsigned char *buf )
{
	unsigned short l = p->length + 5;

	buf[0]   = p->type;
	buf[1]   = p->id;
	buf[2]   = 0;
	buf[3]   = l >> 8;
	buf[4]   = l & 0xff;
	if ( p->length > 0 ) {
		memcpy(&buf[5],p->data,p->length);
	}
	buf[l]   = p->checksum >> 8;
	buf[l+1] = p->checksum & 0xff;

	fprintf(stderr, "\nserialize: type=%hhx\n", p->type);
	fprintf(stderr, "serialize: id=  %hhx\n", p->id);
	fprintf(stderr, "serialize: len= %hx\n", p->length);
	hexdump(p->data, p->length);
  
	return l+2;
}

static void hexdump(void *ptr, int buflen) { 
	unsigned char *buf = (unsigned char*)ptr; 
	int i, j; 
	for (i=0; i<buflen; i+=16) { 
		fprintf(stderr, "%06x: ", i); 
		for (j=0; j<16; j++)  
			if (i+j < buflen) 
				fprintf(stderr, "%02x ", buf[i+j]); 
			else 
				fprintf(stderr, "   "); 
		fprintf(stderr, " "); 
		for (j=0; j<16; j++)  
			if (i+j < buflen) 
				fprintf(stderr, "%c", isprint(buf[i+j]) ? buf[i+j] : '.'); 
		fprintf(stderr, "\n"); 
	} 
} 

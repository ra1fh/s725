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
static int recv_short(unsigned short *s, struct s710_driver *d);
static unsigned short packet_checksum(packet_t *p);
static int packet_serialize(packet_t *p, unsigned char *buf);

/* 
 * send a packet via the S710 driver
 */
int 
packet_send(packet_t *p, struct s710_driver *d)
{
	int           ret = 1;
	unsigned char serialized[BUFSIZ];
	int           bytes;

	/* first, compute the packet checksum */
	p->checksum = packet_checksum(p);
  
	/* next, serialize the packet into a stream of bytes */
	bytes = packet_serialize(p, serialized);

	ret = driver_write(d, serialized, bytes);

	return ret != -1;
}


/*
 * receive a packet from the S710 driver (allocates memory)
 */
packet_t *
packet_recv(struct s710_driver *d) {
	int             r;
	int             i;
	unsigned char   c = 0;
	unsigned char   id;
	unsigned short  len;
	size_t          siz;
	packet_t       *p = NULL;
	unsigned short  crc = 0;

	r = driver_read_byte(d, &c);
	crc_process ( &crc, c );

	if ( c == S710_RESPONSE ) {
		r = driver_read_byte(d, &id);
		crc_process ( &crc, id );
		r = driver_read_byte(d, &c);
		crc_process ( &crc, c );
		r = recv_short ( &len, d );
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
					r = driver_read_byte(d, &p->data[i]);
					crc_process ( &crc, p->data[i] );
					if ( !r ) {
						fprintf(stderr, "driver_read_byte failed\n");
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
recv_short(unsigned short *s, struct s710_driver *d)
{
	int           r = 0;
	unsigned char u = 0;
	unsigned char l = 0;

	r = driver_read_byte(d, &u);
	r = driver_read_byte(d, &l);

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
packet_serialize(packet_t *p, unsigned char *buf)
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

	return l+2;
}

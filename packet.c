/* packet.c - sending/receiving of packets */

/*
 * Copyright (C) 2016  Ralf Horstmann
 * Copyright (C) 2007  Dave Bailey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "log.h"
#include "packet.h"

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

void packet_crc_process(unsigned short *context, unsigned char ch);
void packet_crc_block(unsigned short *context, const unsigned char *blk, int len);

static packet_t gPacket[] = {

	/* packet name, subtype, payload length, checksum value, payload data */
	/* note that checksum values are calculated at packet assembly time.  */

	/* name                type          id    len checksum data          */

	/* S725_GET_OVERVIEW */
	{ "get overview",      S725_REQUEST, 0x15, 0, 0, { 0 } },

	/* S725_GET_USER */
	{ "get user",          S725_REQUEST, 0x06, 0, 0, { 0 } },

	/* S725_GET_WATCH */
	{ "get watch",         S725_REQUEST, 0x02, 0, 0, { 0 } },

	/* S725_GET_LOGO */
	{ "get logo",          S725_REQUEST, 0x10, 0, 0, { 0 } },

	/* S725_GET_BIKE */
	{ "get bike",          S725_REQUEST, 0x14, 0, 0, { 0 } },

	/* S725_GET_EXERCISE_1 */
	{ "get exercise 1",    S725_REQUEST, 0x04, 1, 0, { 0x11 } },

	/* S725_GET_EXERCISE_2 */
	{ "get exercise 2",    S725_REQUEST, 0x04, 1, 0, { 0x22 } },

	/* S725_GET_EXERCISE_3 */
	{ "get exercise 3",    S725_REQUEST, 0x04, 1, 0, { 0x33 } },

	/* S725_GET_EXERCISE_4 */
	{ "get exercise 4",    S725_REQUEST, 0x04, 1, 0, { 0x44 } },

	/* S725_GET_EXERCISE_5 */
	{ "get exercise 5",    S725_REQUEST, 0x04, 1, 0, { 0x55 } },

	/* S725_GET_REMINDER_1 */
	{ "get reminder 1",    S725_REQUEST, 0x0e, 1, 0, { 0x00 } },

	/* S725_GET_REMINDER_2 */
	{ "get reminder 2",    S725_REQUEST, 0x0e, 1, 0, { 0x01 } },

	/* S725_GET_REMINDER_3 */
	{ "get reminder 3",    S725_REQUEST, 0x0e, 1, 0, { 0x02 } },

	/* S725_GET_REMINDER_4 */
	{ "get reminder 4",    S725_REQUEST, 0x0e, 1, 0, { 0x03 } },

	/* S725_GET_REMINDER_5 */
	{ "get reminder 5",    S725_REQUEST, 0x0e, 1, 0, { 0x04 } },

	/* S725_GET_REMINDER_6 */
	{ "get reminder 6",    S725_REQUEST, 0x0e, 1, 0, { 0x05 } },

	/* S725_GET_REMINDER_7 */
	{ "get reminder 7",    S725_REQUEST, 0x0e, 1, 0, { 0x06 } },

	/* S725_GET_FILES */
	{ "get files",         S725_REQUEST, 0x0b, 0, 0, { 0 } },

	/* S725_CONTINUE_TRANSFER */
	{ "continue transfer", S725_REQUEST, 0x16, 0, 0, { 0 } },

	/* S725_CLOSE_CONNECTION */
	{ "close connection",  S725_REQUEST, 0x0a, 0, 0, { 0 } },

	/* S725_SET_USER */
	{ "set user",          S725_REQUEST, 0x05, 21, 0, { 0 } },

	/* S725_SET_WATCH */
	{ "set watch",         S725_REQUEST, 0x01, 11, 0, { 0 } },

	/* S725_SET_LOGO */
	{ "set logo",          S725_REQUEST, 0x0f, 47, 0, { 0 } },

	/* S725_SET_BIKE */
	{ "set bike",          S725_REQUEST, 0x13, 25, 0, { 0 } },

	/* S725_SET_EXERCISE_1 */
	{ "set exercise 1",    S725_REQUEST, 0x03, 23, 0, { 0x11 } },

	/* S725_SET_EXERCISE_2 */
	{ "set exercise 2",    S725_REQUEST, 0x03, 23, 0, { 0x22 } },

	/* S725_SET_EXERCISE_3 */
	{ "set exercise 3",    S725_REQUEST, 0x03, 23, 0, { 0x33 } },

	/* S725_SET_EXERCISE_4 */
	{ "set exercise 4",    S725_REQUEST, 0x03, 23, 0, { 0x44 } },

	/* S725_SET_EXERCISE_5 */
	{ "set exercise 5",    S725_REQUEST, 0x03, 23, 0, { 0x55 } },

	/* S725_SET_REMINDER_1 */
	{ "set reminder 1",    S725_REQUEST, 0x0d, 14, 0, { 0x00 } },

	/* S725_SET_REMINDER_2 */
	{ "set reminder 2",    S725_REQUEST, 0x0d, 14, 0, { 0x01 } },

	/* S725_SET_REMINDER_3 */
	{ "set reminder 3",    S725_REQUEST, 0x0d, 14, 0, { 0x02 } },

	/* S725_SET_REMINDER_4 */
	{ "set reminder 4",    S725_REQUEST, 0x0d, 14, 0, { 0x03 } },

	/* S725_SET_REMINDER_5 */
	{ "set reminder 5",    S725_REQUEST, 0x0d, 14, 0, { 0x04 } },

	/* S725_SET_REMINDER_6 */
	{ "set reminder 6",    S725_REQUEST, 0x0d, 14, 0, { 0x05 } },

	/* S725_SET_REMINDER_7 */
	{ "set reminder 7",    S725_REQUEST, 0x0d, 14, 0, { 0x06 } },

	/* S725_HARD_RESET */
	{ "hard reset",        S725_REQUEST, 0x09, 0, 0x8796, { 0 } }
};

static int gNumPackets = sizeof(gPacket)/sizeof(gPacket[0]);

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

unsigned char
packet_get_type(packet_t *p)
{
	return p->type;
}

/* return a packet pointer for a given packet index */
packet_t *
packet_get(S725_Packet_Index idx)
{
	return (idx > S725_PACKET_INDEX_INVALID && idx < gNumPackets) ? &gPacket[idx] : NULL;
}

/* get a single-packet response to a request */
packet_t *
packet_get_response(S725_Packet_Index request)
{
	packet_t *send = NULL;
	packet_t *recv = NULL;

	if (request == S725_LISTEN) {
		while (1) {
			recv = packet_recv();
			if (recv)
				return(recv);
		}
	}

	send = packet_get(request);

	if (send != NULL && packet_send(send) > 0)
		recv = packet_recv();
	if (recv == NULL) {
		if (send != NULL) {
			if (request != S725_CONTINUE_TRANSFER &&
				 request != S725_CLOSE_CONNECTION) {
				log_write("\n%s: %s\n\n", send->name,
						  (errno) ? strerror(errno) : "No response");
			}
		} else {
			log_write("\n%d: bad packet index\n\n", request);
		}
	}

	return recv;
}

/* This file needs to be rewritten.  */
#define READ_TRIES   10

/* static helper functions */
static int packet_recv_short(unsigned short *s);
static unsigned short packet_checksum(packet_t *p);
static int packet_serialize(packet_t *p, BUF *buf);

/*
 * send a packet via the S725 driver
 */
int
packet_send(packet_t *p)
{
	int  ret = 1;
	BUF *buf;

	buf = buf_alloc(0);
	p->checksum = packet_checksum(p);

	if (driver_uses_frames()) {
		packet_serialize(p, buf);
	} else {
		buf_putc(buf, p->id);
	}
	ret = driver_write(buf);

	buf_free(buf);

	return ret != -1;
}

packet_t * packet_recv_noframes();
packet_t * packet_recv_frames();

packet_t *
packet_recv()
{
	if (driver_uses_frames()) {
		return packet_recv_frames();
	} else {
		return packet_recv_noframes();
	}
}

packet_t *
packet_recv_noframes()
{
	BUF *buf = NULL;
	packet_t *p = NULL;

	buf = buf_alloc(1024);

	if (driver_read(buf) <= 0) {
		log_info("packet_recv_noframes: driver_read returned no data");
		goto error;
	}

	p = calloc(1, sizeof(packet_t) + buf_len(buf));
	if (!p)
		goto error;

	p->type   = S725_RESPONSE;
	p->id     = buf_getc(buf, 0);
	p->length = buf_len(buf);
	memcpy(p->data, buf_get(buf) + 1, buf_len(buf) - 1);

	buf_free(buf);
	return p;

error:
	log_info("packet_recv: error");
	buf_free(buf);
	if (p)
		free(p);
	return NULL;
}

/*
 * receive a packet from the S725 driver (allocates memory)
 */
packet_t *
packet_recv_frames()
{
	BUF *buf;
	int r;
	int i;
	unsigned char c = 0;
	unsigned char id;
	unsigned short len;
	size_t siz;
	packet_t *p = NULL;
	unsigned short crc = 0;

	buf = buf_alloc(0);

	r = driver_read_byte(&c);
	if (r <= 0)
		goto error;
	packet_crc_process(&crc, c);
	buf_putc(buf, c);
	log_info("packet_recv: type=%02hhx", c);

	if (c != S725_RESPONSE) {
		log_info("packet_recv: type != S725_RESPONSE");
		goto error;
	}

	r = driver_read_byte(&id);
	if (r <= 0)
		goto error;

	buf_putc(buf, id);
	log_info("packet_recv: subtype=%02hhx", id);
	packet_crc_process(&crc, id);

	r = driver_read_byte(&c);
	if (r <= 0)
		goto error;
	packet_crc_process(&crc, c);
	buf_putc(buf, c);
	log_info("packet_recv: first=%02x remaining=%02x", c & 0x80, c & 0x7f);

	r = packet_recv_short(&len);
	if (r <= 0)
		goto error;
	log_info("packet_recv: len=%04hx (%hu)", len, len);
	packet_crc_process(&crc, len >> 8);
	packet_crc_process(&crc, len & 0xff);
	buf_putc(buf, len >> 8);
	buf_putc(buf, len & 0xff);
	len -= 5;
	siz = (len <= 1) ? 0 : len - 1;

	p = calloc(1, sizeof(packet_t) + siz);
	if (!p)
		goto error;

	p->type   = S725_RESPONSE;
	p->id     = id;
	p->length = len;
	for (i = 0; i < len; i++) {
		r = driver_read_byte(&p->data[i]);
		packet_crc_process(&crc, p->data[i]);
		buf_putc(buf, p->data[i]);
		if (r <= 0) {
			log_error("driver_read_byte failed");
			free(p);
			p = NULL;
			break;
		}
	}

	if (p == NULL)
		goto error;

	r = packet_recv_short(&p->checksum);
	if (r <= 0) {
		log_error("packet_recv: recv_short failed");
		free(p);
		p = NULL;
		return NULL;
	}

	buf_putc(buf, p->checksum >> 8);
	buf_putc(buf, p->checksum & 0xff);

	if (log_get_level() >= 2) {
		log_info("packet_recv: hexdump len=%zu", buf_len(buf));
		log_hexdump(buf_get(buf), buf_len(buf));
	}

	if (crc != p->checksum) {
		/* discard the whole transmission on crc mismatch,
		   there is no way to retransmit a single packet   */
		log_error("packet_recv: CRC failed [id %d, length %d]",
				  p->id, p->length );
		log_info("packet_recv: reading remaining bytes");
		while (1) {
			r = driver_read_byte(&c);
			if (r <= 0) {
				log_error("packet_recv: driver_read_byte failed (recv_short)");
				break;
			}
			log_info("packet_recv: got byte: %hhx", c);
		}
		goto error;
	}

	log_info("packet_recv: crc correct");
	buf_free(buf);
	return p;

error:
	log_info("packet_recv: error");
	buf_free(buf);
	if (p)
		free(p);
	return NULL;
}

/*
 * read a short from fd
 */
static int
packet_recv_short(unsigned short *s)
{
	int r = 0;
	unsigned char u = 0;
	unsigned char l = 0;

	r = driver_read_byte(&u);
	if (r <= 0)
		return r;
	r = driver_read_byte(&l);
	if (r <= 0)
		return r;
	*s = (unsigned short)(u<<8)|l;

	return r;
}

static unsigned short
packet_checksum(packet_t *p)
{
	unsigned short crc = 0;

	packet_crc_process(&crc, S725_REQUEST);
	packet_crc_process(&crc, p->id);
	packet_crc_process(&crc, 0);
	packet_crc_process(&crc, (p->length + 5) >> 8);
	packet_crc_process(&crc, (p->length + 5) & 0xff);
	packet_crc_block(&crc, p->data, p->length);
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
	if (p->length > 0)
		buf_append(buf, p->data, p->length);
	buf_putc(buf, p->checksum >> 8);
	buf_putc(buf, p->checksum & 0xff);

	return buf_len(buf);
}

/*
 * Thanks to Stefan Kleditzsch for decoding the checksum algorithm!
 */

/*
 * crc16 checksum function with polynom=0x8005
 */
void
packet_crc_process(unsigned short *context, unsigned char ch)
{
	unsigned short uch  = (unsigned short) ch;
	int i;

	*context ^= (uch << 8);

	for (i = 0; i < 8; i++) {
		if (*context & 0x8000)
			*context = (*context << 1)^0x8005;
		else
			*context <<= 1;
	}
}

void
packet_crc_block(unsigned short *context, const unsigned char *blk, int len)
{
	while (len -- > 0)
		packet_crc_process(context, * blk ++);
}

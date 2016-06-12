/* files.c - high level functions for receiving and saving files */

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

#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buf.h"
#include "files.h"
#include "log.h"
#include "packet.h"

static int files_transfer(BUF *files, int packet_type);

/* 
 * Send read request and receive training data. Each packet contains
 * the number of remaining packets.  To proceed with transfer, a
 * continue request has to be sent.
 */
int
files_get(BUF *files)
{
	return files_transfer(files, S725_GET_FILES);
}

/* 
 * Listen for incoming training data. The send operation has to be
 * initiated from the watch
 */
int
files_listen(BUF *files)
{
	return files_transfer(files, S725_LISTEN);
}

int
files_split(BUF *files, int *offset, BUF *out)
{
	int	size;
	u_char *bp;

	if (*offset < buf_len(files) - 2) {
		size = (buf_getc(files, *offset + 1) << 8) + buf_getc(files, *offset);
		buf_empty(out);
		bp = buf_get(files);
		buf_append(out, &bp[*offset], size);
		*offset += size;
		return 1;
	}
	return 0;
}

time_t
files_timestamp (BUF *f, size_t offset)
{
	struct tm t;
	time_t ft;

	t.tm_sec   = buf_getbcd(f, offset + 10);
	t.tm_min   = buf_getbcd(f, offset + 11);
	t.tm_hour  = buf_getbcd_masked(f, offset + 12, 0x7f);

	/* PATCH for AM/PM mode detection from Berend Ozceri */
	t.tm_hour += (buf_getc(f, offset + 13) & 0x80) ?                       /* am/pm mode?   */
		((buf_getc(f, offset + 12) & 0x80) ? ((t.tm_hour < 12) ? 12 : 0) : /* yes, pm set   */
		 ((t.tm_hour >= 12) ? -12 : 0)) :                  /* yes, pm unset */
		0;                                                 /* no            */

	t.tm_mon   = (buf_getc(f, offset + 15) - 1) &0x0f;
	t.tm_mday  = buf_getbcd_masked(f, offset + 13, 0x7f);
	t.tm_year  = 100 + buf_getbcd(f, offset + 14);
	t.tm_isdst = -1;
	ft         = mktime(&t);
  
	return ft;
}

static int
files_transfer(BUF *files, int packet_type)
{
	packet_t *p;
	int p_remaining = 1;
	int p_first = 0;
	unsigned short p_bytes = 0;
	unsigned int start;

	buf_empty(files);

	log_write("Reading ");
	log_prep_hash_marks();
	log_print_hash_marks(0, 0);

	while (p_remaining) {
		p = packet_get_response(packet_type);
		if (p == NULL) {
			log_write("[error]\n");
			return 0;
		}
		/* Bit 8: first packet, Bit 7-1: packets remaining */
		p_first		= packet_data(p)[0] & 0x80;
		p_remaining = packet_data(p)[0] & 0x7f;
		if (p_first) {
			/* Byte 1 and 2 of first packet: total size in bytes */
			p_bytes = (packet_data(p)[1] << 8) + packet_data(p)[2];
			/* Byte 3 and 4 of first packet: magic bytes */
			start = 5;
		} else {
			start = 1;
		}

		unsigned char *pd = packet_data(p);
		int len = packet_len(p);
		buf_append(files, &pd[start], len - start);

		if (p_bytes > 0) 
			log_print_hash_marks(buf_len(files) * 100 / p_bytes, p_bytes);

		if (packet_type == S725_GET_FILES)
			packet_type = S725_CONTINUE_TRANSFER;

		/* free this packet and get the next one */
		free(p);
	}

	if (p_remaining != 0) {
		log_write("[error]\n");
		return 0;
	}

	log_write("\n");
	return 1;
}

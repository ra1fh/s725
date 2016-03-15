/*
 * Copyright (c) 2016 Ralf Horstmann <ralf@ackstorm.de>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "files.h"
#include "packet.h"
#include "utils.h"

int time_get() {
	packet_t *p = NULL;

	p = packet_get_response(S725_GET_WATCH);
	if (p == NULL) {
		printf("[error]\n");
		return 0;
	}

	u_char *data;
	if ((data = packet_data(p)) && packet_len(p) > 5) {
		fprintf(stderr,"20%02x-%02x-%02xT%02x:%02x:%02x\n",
				data[4], data[5]&0x0F, data[3],
				data[2], data[1], data[0]);
	}
	free(p);

	return 0;
}

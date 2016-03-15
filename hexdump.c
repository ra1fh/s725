/*
 * Based on:  OpenBSD src/usr.sbin/iscsid/log.c 1.6
 *
 * Copyright (c) 2009 Claudio Jeker <claudio@openbsd.org>
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
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

#include "hexdump.h"
#include <stdio.h>
#include <string.h>

void
hexdump(void *buf, size_t len)
{
	unsigned char b[16];
	size_t i, j, l;

	for (i = 0; i < len; i += l) {
		fprintf(stderr, "%4zi:", i);
		l = sizeof(b) < len - i ? sizeof(b) : len - i;
		memcpy(b, (char *)buf + i, l);

		for (j = 0; j < sizeof(b); j++) {
			if (j % 2 == 0)
				fprintf(stderr, " ");
			if (j % 8 == 0)
				fprintf(stderr, " ");
			if (j < l)
				fprintf(stderr, "%02x", (int)b[j]);
			else
				fprintf(stderr, "  ");
		}
		fprintf(stderr, "  |");
		for (j = 0; j < l; j++) {
			if (b[j] >= 0x20 && b[j] <= 0x7e)
				fprintf(stderr, "%c", b[j]);
			else
				fprintf(stderr, ".");
		}
		fprintf(stderr, "|\n");
	}
}

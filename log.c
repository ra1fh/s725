/* log.c - logging api */

/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 2007 Nicholas Marriott <nicholas.marriott@gmail.com>
 * Copyright (c) 2009 Claudio Jeker <claudio@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"

static FILE *log_file;
static int log_level;

#define HASH_MARKS   40

static void log_vwrite(int, const char *, va_list);

/* Increment log level. */
void
log_add_level(void)
{
	log_level++;
}

/* Get log level. */
int
log_get_level(void)
{
	return (log_level);
}

/* Open logging to file. */
void
log_open(const char *name)
{
	if (log_file != NULL) {
		fclose(log_file);
		log_file = NULL;
	}

	if (name != NULL)
		log_file = fopen(name, "w");
}

/* Close logging. */
void
log_close(void)
{
	if (log_file != NULL)
		fclose(log_file);
	log_file = NULL;

}

/* Write a log message. */
static void
log_vwrite(int newline, const char *msg, va_list ap)
{
	char *fmt;

	if (vasprintf(&fmt, msg, ap) == -1)
		exit(1);

	if (log_file != NULL) {
		if (fprintf(log_file, "%s%s", fmt, newline ? "\n" : "") == -1)
			exit(1);
		fflush(log_file);
	} else {
		if (fprintf(stderr, "%s%s", fmt, newline ? "\n" : "") == -1)
			exit(1);
	}

	free(fmt);
}

/* Write hexdump of a buffer */
void
log_hexdump(void *buf, size_t len)
{
	unsigned char b[16];
	size_t i, j, l;

	for (i = 0; i < len; i += l) {
		log_write("%4zi:", i);
		l = sizeof(b) < len - i ? sizeof(b) : len - i;
		memcpy(b, (char *)buf + i, l);

		for (j = 0; j < sizeof(b); j++) {
			if (j % 2 == 0)
				log_write(" ");
			if (j % 8 == 0)
				log_write(" ");
			if (j < l)
				log_write("%02x", (int)b[j]);
			else
				log_write("  ");
		}
		log_write("  |");
		for (j = 0; j < l; j++) {
			if (b[j] >= 0x20 && b[j] <= 0x7e)
				log_write("%c", b[j]);
			else
				log_write(".");
		}
		log_writeln("|");
	}
}

/* Write log message without newline */
void
log_write(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	log_vwrite(0, msg, ap);
	va_end(ap);
}


/* Write log message with newline */
void
log_writeln(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	log_vwrite(1, msg, ap);
	va_end(ap);
}

/* Write error message */
void
log_error(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	log_vwrite(1, msg, ap);
	va_end(ap);
}

/* Log a info message. */
void
log_info(const char *msg, ...)
{
	va_list ap;

	if (log_level < 1)
		return;

	va_start(ap, msg);
	log_vwrite(1, msg, ap);
	va_end(ap);
}

/* Log a debug message. */
void
log_debug(const char *msg, ...)
{
	va_list ap;

	if (log_level < 2)
		return;

	va_start(ap, msg);
	log_vwrite(1, msg, ap);
	va_end(ap);
}

/* Log a critical error with error string and die. */
void
fatal(const char *msg, ...)
{
	char *fmt;
	va_list ap;

	va_start(ap, msg);
	if (asprintf(&fmt, "fatal: %s: %s", msg, strerror(errno)) == -1)
		exit(1);
	log_vwrite(1, fmt, ap);
	exit(1);
}

/* Log a critical error and die. */
void
fatalx(const char *msg, ...)
{
	char *fmt;
	va_list ap;

	va_start(ap, msg);
	if (asprintf(&fmt, "fatal: %s", msg) == -1)
		exit(1);
	log_vwrite(1, fmt, ap);
	exit(1);
}

void
log_prep_hash_marks(void)
{
	int i;

	if (log_level != 0 || log_file != NULL)
		return;

	log_write("[%5d bytes] [",0);
	for (i = 0; i < HASH_MARKS; i++)
		log_write(" ");
	log_write("] [%5d%%]", 0);
}

void
log_print_hash_marks(int pct, int bytes)
{
	float here;
	int i;

	if (log_level != 0 || log_file != NULL)
		return;

	for (i = 0; i < HASH_MARKS+25; i++)
		log_write("\b");
	log_write("[%5d bytes] [", bytes);
	for (i = 0; i < HASH_MARKS; i++) {
		here = i * 100 / HASH_MARKS;
		if (here < pct)
			log_write("#");
		else
			log_write(" ");
	}
	log_write("] [%5d%%]", pct);
}

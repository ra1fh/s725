
#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"
#include "files.h"
#include "packet.h"
#include "utils.h"

#define HASH_MARKS   40

static void   files_prep_hash_marks();
static void   files_print_hash_marks(int pct, int bytes);

/* 
 * This function reads the user's workout data from the watch and stores
 * it in a structure which is nothing more than a giant byte array.  The
 * function, unfortunately, is error-prone because we don't have any good
 * way to check integrity of the data.  
 */
int
files_get(BUF *files)
{
	packet_t      *p;
	int            p_remaining = 1;
	int            p_first = 0;
	unsigned short p_bytes = 0;
	unsigned int   start;

	buf_empty(files);

	printf("\nReading ");
	files_prep_hash_marks();
	files_print_hash_marks(0, 0);

	p = packet_get_response(S710_GET_FILES);
	
	if (p == NULL) {
		printf("[error]");
		return 0;
	}

	while (p != NULL) {
		/* Bit 8: first packet, Bit 7-1: packets remaining */
		p_first     = packet_data(p)[0] & 0x80;
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
			files_print_hash_marks(buf_len(files) * 100 / p_bytes, p_bytes);

		/* free this packet and get the next one */
		free(p);
		p = packet_get_response(S710_CONTINUE_TRANSFER);
	}

	printf("\n\n");

	if (p_remaining != 0)
		return 0;

	return 1;
}

int
files_split(BUF *files, int *offset, BUF *out)
{
	int         size;
	char       *bp;

	if (*offset < buf_len(files) - 2) {
		size  = (buf_getc(files, *offset + 1) << 8) + buf_getc(files, *offset);
		buf_empty(out);
		bp = buf_get(files);
		buf_append(out, &bp[*offset], size);
		*offset += size;
		return 1;
	}
	return 0;
}

int
files_save(BUF *files, const char *dir)
{
	int         saved  = 0;
	int         offset = 0;
	int         size;
	char        buf[BUFSIZ];
	char        tmbuf[128];
	time_t      ft;
	int         ofd;
	uid_t       owner = 0;
	gid_t       group = 0;
	char       *bp;

	while (offset < buf_len(files) - 2) {
		size  = (buf_getc(files, offset + 1) << 8) + buf_getc(files, offset);

		ft    = files_timestamp(files, offset);
		strftime(tmbuf,sizeof(tmbuf),"%Y%m%dT%H%M%S", localtime(&ft));
		snprintf(buf, sizeof(buf), "%s/%s.%05d.srd",dir,tmbuf,size);

		ofd = open(buf, O_CREAT|O_WRONLY, 0644);
		if (ofd != -1) {
			printf("File %02d: Saved as %s\n", saved+1,buf);
			bp = buf_get(files);
			write(ofd, &bp[offset], size);
			fchown(ofd, owner, group);
			close(ofd);
		} else {
			printf("File %02d: Unable to save %s: %s\n",
				   saved+1,buf,strerror(errno));
		}

		offset += size;
		saved++;
	}
	printf("Saved %d file%s\n",saved,(saved==1)?"":"s");

	return saved;
}

time_t
files_timestamp (BUF *f, size_t offset)
{
	struct tm t;
	time_t    ft;

	t.tm_sec   = BCD(buf_getc(f, offset + 10));
	t.tm_min   = BCD(buf_getc(f, offset + 11));
	t.tm_hour  = BCD(buf_getc(f, offset + 12) & 0x7f);

	/* PATCH for AM/PM mode detection from Berend Ozceri */
	t.tm_hour += (buf_getc(f, offset + 13) & 0x80) ?                       /* am/pm mode?   */
		((buf_getc(f, offset + 12) & 0x80) ? ((t.tm_hour < 12) ? 12 : 0) : /* yes, pm set   */
		 ((t.tm_hour >= 12) ? -12 : 0)) :                  /* yes, pm unset */
		0;                                                 /* no            */

	t.tm_mon   = LNIB(buf_getc(f, offset + 15) - 1);
	t.tm_mday  = BCD(buf_getc(f, offset + 13) & 0x7f);
	t.tm_year  = 100 + BCD(buf_getc(f, offset + 14));
	t.tm_isdst = -1;
	ft         = mktime(&t);
  
	return ft;
}

static void
files_prep_hash_marks()
{
	int i;

	printf("[%5d bytes] [",0);
	for (i = 0; i < HASH_MARKS; i++)
		putchar(' ');
	printf("] [%5d%%]", 0);
	fflush(stdout);
}

static void
files_print_hash_marks(int pct, int bytes)
{
	float here;
	int   i;

	for (i = 0; i < HASH_MARKS+25; i++)
		putchar('\b');
	printf("[%5d bytes] [", bytes);
	for (i = 0; i < HASH_MARKS; i++) {
		here = i * 100 / HASH_MARKS;
		if (here < pct)
			putchar('#');
		else
			putchar(' ');
	}
	printf("] [%5d%%]", pct);
	fflush(stdout);
}

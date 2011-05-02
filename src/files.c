#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "s710.h"


#define HASH_MARKS   40


static void   prep_hash_marks  ( FILE *fp );
static void   print_hash_marks ( float pct, int bytes, FILE *fp );

/* 
   This function reads the user's workout data from the watch and stores
   it in a structure which is nothing more than a giant byte array.  The
   function, unfortunately, is error-prone because we don't have any good
   way to check integrity of the data.  
*/

int
get_files ( S710_Driver *d, files_t *files, FILE *fp )
{
	packet_t      *p;
	int            ok = 0;
	int            p_remaining = 1;
	unsigned int   start;
	unsigned int   offset = 0;

	/* send the first packet - S710_GET_FILES */

	if ( fp != NULL ) {
		fprintf(fp,"\nReading ");
		prep_hash_marks(fp);
		print_hash_marks(0,0,fp);
	}

	p = get_response(S710_GET_FILES,d);
	files->bytes = 0;
	files->cursor = 0;

	if ( p != NULL ) ok = 1;
	else if ( fp != NULL ) fprintf(fp,"[error]");

	while ( p != NULL ) {

		/* handle this packet */

		p_remaining = p->data[0] & 0x7f;
		if ( p->data[0] & 0x80 ) {
			files->bytes = (p->data[1] << 8) + p->data[2];
			start = 5;
		} else {
			start = 1;
		}

		memcpy(&files->data[offset],&p->data[start],p->length - start);
		offset += p->length - start;
		if ( fp != NULL && files->bytes > 0 ) 
			print_hash_marks((float)offset/files->bytes,files->bytes,fp);

		/* free this packet and get the next one */

		free ( p );
		p = get_response(S710_CONTINUE_TRANSFER,d);
	}

	if ( fp != NULL ) fprintf(fp,"\n\n");
	if ( p_remaining != 0 ) ok = 0;

	return ok;
}


int
receive_file(S710_Driver *d, files_t *file, log_cb* cb)
{
	packet_t      *p;
	int            ok = 0;
	int            p_remaining = 1;
	unsigned int   start;
	unsigned int   offset = 0;

	p = recv_packet(d);
	file->bytes = 0;
	file->cursor = 0;

	if ( p != NULL ) ok = 1;

	while ( p != NULL ) {

		/* handle this packet */

		p_remaining = p->data[0] & 0x7f;
		if ( p->data[0] & 0x80 ) {
			file->bytes = (p->data[1] << 8) + p->data[2];
			start = 5;
		} else {
			start = 1;
		}

		memcpy(&file->data[offset],&p->data[start],p->length - start);
		offset += p->length - start;
		if (cb) {
			cb(1, "transferred %d/%d bytes\n", offset, file->bytes);
		}

		/* free this packet and get the next one */

		free ( p );
		p = recv_packet(d);
	}

	if ( p_remaining != 0 ) ok = 0;

	return ok;
}




/* 
   FIX ME!!!! 

   This function doesn't do anything yet.  It's designed to operate both
   on the files_t that's filled in by get_files() and the files_t we'd get
   if we just slurped in a single file from disk.
*/

void
print_files(files_t *f, log_cb *cb)
{
	int       offset = 0;
	int       size;
	time_t    ft;
	int       hours;
	int       minutes;
	int       seconds;
	int       tenths;
	int       fnum = 0;
	char      buf[BUFSIZ];

	while ( offset < f->bytes-2 ) {
		size = (f->data[offset+1] << 8) + f->data[offset];
		ft   = file_timestamp(&f->data[offset]);
		strftime(buf,sizeof(buf),"%a, %d %b %Y %T",localtime(&ft));
		hours      = BCD(f->data[offset+18]);
		minutes    = BCD(f->data[offset+17]);
		seconds    = BCD(f->data[offset+16]);
		tenths     = UNIB(f->data[offset+15]);
		if (cb) {
			cb(1, "File %02d: %s - %02d:%02d:%02d.%d\n",
			   ++fnum,
			   buf,
			   hours,
			   minutes,
			   seconds,
			   tenths);
		}
		offset += size;
	}
}


int
save_files ( files_t *f, const char *dir, log_cb *cb)
{
	int         saved  = 0;
	int         offset = 0;
	int         size;
	char        buf[BUFSIZ];
	char        tmbuf[128];
	time_t      ft;
	int         ofd;
	int         year;
	int         month;
	uid_t       owner = 0;
	gid_t       group = 0;


	while ( offset < f->bytes-2 ) {
		size  = (f->data[offset+1] << 8) + f->data[offset];
		ft    = file_timestamp(&f->data[offset]);
		year  = 2000 + BCD(f->data[offset+14]);
		month = LNIB(f->data[offset+15]);

		strftime(tmbuf,sizeof(tmbuf),"%Y%m%dT%H%M%S", localtime(&ft));

		snprintf(buf, sizeof(buf), "%s/%s.%05d.srd",dir,tmbuf,size);
		ofd = open(buf,O_CREAT|O_WRONLY,0644);
		if ( ofd != -1 ) {
			if (cb)
				cb(0, "File %02d: Saved as %s\n",saved+1,buf);
			write(ofd,&f->data[offset],size);
			fchown(ofd,owner,group);
			close(ofd);
		} else {
			if (cb) {
				cb(0, "File %02d: Unable to save %s: %s\n",
				   saved+1,buf,strerror(errno));
			}
		}

		offset += size;
		saved++;
	}

	if (cb) 
		cb(1,"Saved %d file%s\n",saved,(saved==1)?"":"s");

	return saved;
}


time_t
file_timestamp ( unsigned char *data )
{
	struct tm t;
	time_t    ft;

	t.tm_sec   = BCD(data[10]);
	t.tm_min   = BCD(data[11]);
	t.tm_hour  = BCD(data[12] & 0x7f);

	/* PATCH for AM/PM mode detection from Berend Ozceri */

	t.tm_hour += (data[13] & 0x80) ?                     /* am/pm mode?   */
		((data[12] & 0x80) ? ((t.tm_hour < 12) ? 12 : 0) : /* yes, pm set   */
		 ((t.tm_hour >= 12) ? -12 : 0)) :                  /* yes, pm unset */
		0;                                                 /* no            */

	t.tm_mon   = LNIB(data[15]) - 1;
	t.tm_mday  = BCD(data[13] & 0x7f);
	t.tm_year  = 100 + BCD(data[14]);
	t.tm_isdst = -1;
	ft         = mktime(&t);
  
	return ft;
}


static void
prep_hash_marks ( FILE *fp )
{
	int i;

	fprintf(fp,"[%5d bytes] [",0);
	for ( i = 0; i < HASH_MARKS; i++ )
		fputc(' ',fp);
	fprintf(fp,"] [%5.1f%%]",0.0);
	fflush(fp);
}


static void
print_hash_marks ( float pct, int bytes, FILE *fp )
{
	float here;
	int   i;

	for ( i = 0; i < HASH_MARKS+25; i++ )
		fputc('\b',fp);
	fprintf(fp,"[%5d bytes] [",bytes);
	for ( i = 0; i < HASH_MARKS; i++ ) {
		here = (float)i/HASH_MARKS;
		if ( here < pct ) fputc('#',fp);
		else              fputc(' ',fp);
	}
	fprintf(fp,"] [%5.1f%%]",pct * 100.0);
	fflush(fp);
}

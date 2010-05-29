/* $Id: srdmerge.c,v 1.1 2004/11/23 18:55:22 dave Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "s710.h"

static void usage(void);

/* externs */

extern char *optarg;
extern int   optind;

static void
usage(void) {
	fprintf(stderr,
			"usage: srdmerge [-c] [-f] [-h] <srd 1> <srd 2> <output>\n");
}

int
main ( int argc, char **argv )
{
	workout_t *      w;
	workout_t *      w1;
	workout_t *      w2;
	int              ch;
	S710_Filter      filter = S710_FILTER_OFF;
	S710_Merge_Type  mtype = S710_MERGE_TRUE;

	while ( (ch = getopt(argc,argv,"cfh")) != -1 ) {
		switch (ch) {
		case 'c': mtype  = S710_MERGE_CONCAT; break;
		case 'f': filter = S710_FILTER_ON;    break;
		case 'h': usage(); exit(0);           break;
		default:                              break;
		}
	}
	argc -= optind;
	argv += optind;

	if ( argc != 3 ) {
		usage();
		exit(1);
	}

	w1 = read_workout(argv[0],filter,S710_HRM_AUTO);
	w2 = read_workout(argv[1],filter,S710_HRM_AUTO);
	if ( w1 != NULL && w2 != NULL ) {
		w = merge_workouts(w1,w2,mtype);
		if ( w != NULL ) {
			if ( w->bytes <= 0xffff ) {
				write_workout(w,argv[2]);
			} else {
				fprintf(stderr,"srdmerge: combined file would be too large.\n");
			}
			free_workout(w);
		} else {
			fprintf(stderr,"srdmerge: unable to merge workouts.\n");
		}
		free_workout(w1);
		free_workout(w2);
	} else {
		fprintf(stderr,"srdmerge: unable to read workouts.\n");
	}

	return 0;
}

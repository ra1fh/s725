#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "s710.h"

static void usage(void);

/* externs */

extern char *optarg;
extern int   optind;

static void
usage(void) {
	fprintf(stderr, "usage: srdcat [-fh] <srd...>\n");
}

int
main ( int argc, char **argv )
{
	int              i;
	workout_t       *w;
	struct timeval   ti;
	struct timeval   tf;
	float            el;
	int              ch;

	while ( (ch = getopt(argc,argv,"h")) != -1 ) {
		switch (ch) {
		case 'h':
			usage();
			exit(0);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	for ( i = 0; i < argc; i++ ) {
		gettimeofday(&ti, NULL);
		w = workout_read(argv[i], S710_HRM_AUTO);
		gettimeofday(&tf, NULL);
		el = tf.tv_sec - ti.tv_sec + (tf.tv_usec-ti.tv_usec)/1000000.0;
		if ( w != NULL ) {
			printf("\nPrinting workout in %s [loaded in %f seconds]:\n\n",
				   argv[i],el);
			workout_print(w,stdout,S710_WORKOUT_FULL);
			workout_free(w);
		} else {
			printf("%s: invalid file\n",argv[i]);
		}
	}

	return 0;
}

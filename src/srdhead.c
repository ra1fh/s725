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
  fprintf(stderr, "usage: srdhead [-fh] <srd...>\n");
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
  S710_Filter      filter = S710_FILTER_OFF;

  while ( (ch = getopt(argc,argv,"fh")) != -1 ) {
    switch (ch) {
    case 'f':
      filter = S710_FILTER_ON;
      break;
    case 'h':
      usage();
      exit(0);
      break;
    }
  }
  argc -= optind;
  argv += optind;

  for ( i = 0; i < argc; i++ ) {
    gettimeofday(&ti,NULL);
    w = read_workout(argv[i],filter,S710_HRM_AUTO);
    gettimeofday(&tf,NULL);
    el = tf.tv_sec - ti.tv_sec + (tf.tv_usec-ti.tv_usec)/1000000.0;
    if ( w != NULL ) {
      printf("\nPrinting workout header in %s [loaded in %f seconds]:\n\n",
	     argv[i],el);
      print_workout(w,stdout,S710_WORKOUT_HEADER|S710_WORKOUT_LAPS);
      free_workout(w);
    } else {
      printf("%s: invalid file\n",argv[i]);
    }
  }

  return 0;
}

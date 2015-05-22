#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "workout.h"
#include "workout_print.h"

static void usage(void);

/* externs */

extern char *optarg;
extern int optind;

static void
usage(void) {
	fprintf(stderr, "usage: srdtxc [-h] <srd...>\n");
}

int
main(int argc, char **argv)
{
	int i;
	workout_t *w;
	int ch;

	while ((ch = getopt(argc, argv, "h")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			exit(0);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	for (i = 0; i < argc; i++) {
		w = workout_read(argv[i]);
		if (w != NULL) {
			workout_print_tcx(w, stdout);
			workout_free(w);
		} else {
			fprintf(stderr, "%s: invalid file\n", argv[i]);
		}
	}

	return 0;
}

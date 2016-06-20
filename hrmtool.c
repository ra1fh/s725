/* hrmtool.c - hrmtool main */

/*
 * Copyright (C) 2016  Ralf Horstmann
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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "conf.h"
#include "driver.h"
#include "files.h"
#include "format.h"
#include "log.h"
#include "misc.h"
#include "workout.h"
#include "workout_print.h"

static void
usage(void) {
	printf("usage: hrmtool [options] [-i intype] [-o outtype] [-f infile] [-F outfile]\n");
	printf("        -i intype      input file type: hrm, srd, tcx, txt\n");
	printf("        -o outtype     output file type: hrm, srd, tcx, txt\n");
	printf("        -f infile      input file name\n");
	printf("        -F outfile     output file name\n");
	printf("        -v             verbose output\n");
}

int
main(int argc, char **argv)
{
	char *opt_input_file = NULL;
	char *opt_output_file = NULL;
	char *opt_input_type = NULL;
	char *opt_output_type = NULL;
	int ch;
	workout_t *w;
	FILE *f;

	while ((ch = getopt(argc, argv, "i:o:f:F:vh")) != -1) {
		switch (ch) {
		case 'i':
			opt_input_type = optarg;
			break;
		case 'o':
			opt_output_type = optarg;
			break;
		case 'f':
			opt_input_file = optarg;
			break;
		case 'F':
			opt_output_file = optarg;
			break;
		case 'v':
			log_add_level();
			break;
		case 'h':
			usage();
			return 0;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (opt_input_type == NULL || opt_output_type == NULL ) {
		usage();
		return 1;
	}

	int input_type = format_from_str(opt_input_type);
	int output_type = format_from_str(opt_output_type);

	if (input_type != FORMAT_SRD) {
		usage();
		return 1;
	}

	if (output_type != FORMAT_TXT && output_type != FORMAT_TCX && output_type != FORMAT_HRM) {
		usage();
		return 1;
	}

	if (opt_input_file == NULL || opt_output_file == NULL ) {
		usage();
		return 1;
	}

	w = workout_read(opt_input_file);
	if (w != NULL) {
		f = fopen(opt_output_file, "w");
		if (f) {
			if (output_type == FORMAT_TCX) {
				workout_print_tcx(w, f);
			} else if (output_type == FORMAT_TXT) {
				workout_print_txt(w, f, S725_WORKOUT_FULL);
			} else if (output_type == FORMAT_HRM) {
				workout_print_hrm(w, f);
			}
			fclose(f);
		}
	} else {
		fatalx("%s: invalid file\n", opt_input_file);
	}
	workout_free(w);

	return 0;
}

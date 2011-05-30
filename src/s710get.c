
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "driver.h"
#include "files.h"

static void     usage(void);

static void
usage(void) {
	printf("usage: s710get [-h] [-d driver] [-f filedir] [device file]\n");
	printf("       driver       may be either serial, ir, or usb. default: ir.\n");
	printf("       filedir      is the directory where output files are written to.\n");
	printf("                    alternative is S710_FILEDIR environment variable.\n");
	printf("                    default: current working directory\n");
	printf("       device file  is required for serial and ir drivers.\n");
}

int
main(int argc, char **argv)
{
	char			  path[PATH_MAX];
	const char		 *opt_filedir = NULL;
	const char		 *opt_driver_name = "ir";
	const char		 *opt_device = NULL;
	BUF              *files;
	int				  opt_raw = 0;
	int				  ch;
	int				  ok;

	while ( (ch = getopt(argc,argv,"d:f:hr")) != -1 ) {
		switch (ch) {
		case 'd':
			opt_driver_name = optarg;
			break;
		case 'f':
			opt_filedir = optarg;
			break;
		case 'r':
			opt_raw = 1;
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
	argc -= optind;
	argv += optind;
	opt_device = argv[0];

	ok = driver_init (opt_driver_name , opt_device);

	if ( ok != 1 ) {
		printf("problem with driver_init\n");
		usage();
		return 1;
	}

	if (opt_filedir) {
		opt_filedir = realpath(opt_filedir, path);
	} else {
		getcwd(path, sizeof(path));
		opt_filedir = path;
	}

	if (!opt_filedir) {
		printf("could not resolve path. check -f\n");
		return 1;
	}

	if (driver_open() < 0) {
		fprintf(stderr,"unable to open port: %s\n",strerror(errno));
		return 1;
	}

	files = buf_alloc(0);

	if (files_get(files)) {
		files_save(files, opt_filedir);
	}

	buf_free(files);
	driver_close();
	return 0;
}

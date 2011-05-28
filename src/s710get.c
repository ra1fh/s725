
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
	const char		 *filedir = NULL;
	char			  path[PATH_MAX];
	int				  ok;
	const char		 *driver_name = "ir";
	const char		 *device = NULL;
	int				  ch;
	BUF              *files;

	while ( (ch = getopt(argc,argv,"d:f:h")) != -1 ) {
		switch (ch) {
		case 'd':
			driver_name = optarg;
			break;
		case 'f':
			filedir = optarg;
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
	device = argv[0];

	ok = driver_init (driver_name , device);

	if ( ok != 1 ) {
		printf("problem with driver_init\n");
		usage();
		return 1;
	}

	if (filedir) {
		filedir = realpath(filedir, path);
	} else if ((filedir = getenv("S710_FILEDIR")) != NULL) {
		filedir = realpath(filedir,path);
	} else {
		getcwd(path, sizeof(path));
		filedir = path;
	}

	if (!filedir) {
		printf("could not resolve path. check S710_FILEDIR or -f\n");
		return 1;
	}

	if (driver_open() < 0) {
		fprintf(stderr,"unable to open port: %s\n",strerror(errno));
		return 1;
	}

	files = buf_alloc(0);

	if (files_get(files)) {
		files_save(files, filedir);
	}

	buf_free(files);
	driver_close();
	return 0;
}

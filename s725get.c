
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>

#include "driver.h"
#include "files.h"
#include "workout.h"

static void     usage(void);

static void
usage(void) {
	printf("usage: s725get [-h] [-d driver] [-f filedir] [device file]\n");
	printf("       driver       may be either serial, ir, or usb. default: ir.\n");
	printf("       filedir      is the directory where output files are written to.\n");
	printf("                    alternative is S725_FILEDIR environment variable.\n");
	printf("                    default: current working directory\n");
	printf("       device file  is required for serial and ir drivers.\n");
}

int
main(int argc, char **argv)
{
	char			  path[PATH_MAX];
	char              inipath[PATH_MAX];
	const char		 *opt_filedir = NULL;
	const char		 *opt_driver_name = "ir";
	const char		 *opt_device = NULL;
	BUF              *files;
	BUF              *buf;
	workout_t        *w;
	int               offset;
	int				  opt_raw = 0;
	int				  ch;
	int				  ok;
	time_t  	     ft;
	FILE*             f;
	GKeyFile         *keyfile;

	keyfile = g_key_file_new();
	snprintf(inipath, PATH_MAX, "%s/.s725getrc", getenv("HOME"));
	if (g_key_file_load_from_file(keyfile, inipath, G_KEY_FILE_NONE, NULL)) {
		opt_device = g_key_file_get_string(keyfile, "main", "device", NULL);
		opt_driver_name = g_key_file_get_string(keyfile, "main", "driver", NULL);
		if (!opt_driver_name)
			opt_driver_name = "ir";
	} else {
		fprintf(stderr, "failed to parse %s\n", inipath);
	}

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
	if (argv[0]) 
		opt_device = argv[0];
	
	fprintf(stderr, "opt_device='%s'\n", opt_device);
	fprintf(stderr, "opt_driver='%s'\n", opt_driver_name);

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
		if (opt_raw) {
			files_save(files, opt_filedir);
		} else {
			buf = buf_alloc(0);
			offset = 0;
			while (files_split(files, &offset, buf)) {
				char        tmbuf[128];
				char        fnbuf[BUFSIZ];

				w = workout_read_buf(buf);
				ft = files_timestamp(buf, 0);
				strftime(tmbuf,sizeof(tmbuf),"%Y%m%dT%H%M%S", localtime(&ft));
				snprintf(fnbuf, sizeof(fnbuf), "%s/%s.%05zd.txt",opt_filedir,tmbuf, buf_len(buf));

				if (w) {
					f = fopen(fnbuf, "w");
					fprintf(stderr, "opened %s for writing\n", fnbuf);
					if (f) {
						workout_print(w, f, S725_WORKOUT_FULL);
					} else {
						fprintf(stderr, "failed to open %s for writing\n", fnbuf);
					}
					workout_free(w);
				}
			}
		}
	}

	buf_free(files);
	driver_close();
	return 0;
}


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
#include "workout.h"
#include "workout_print.h"

static void
usage(void) {
	printf("usage: s725get [-hHrv] [-d driver] [-D device] [-f directory]\n");
	printf("        -d driver      driver type: serial, ir, or usb. (default: ir).\n");
	printf("        -D device      device file. required for serial and ir driver.\n");
	printf("        -f directory   directory where output files are written to.\n");
	printf("                       default: current working directory\n");
	printf("        -H             write HRM format\n");
	printf("        -r             write raw srd format\n");
	printf("        -v             verbose output\n");
}

int
main(int argc, char **argv)
{
	char			  path[PATH_MAX];
	char			  inipath[PATH_MAX];
	const char		 *opt_directory_name = NULL;
	const char		 *opt_driver_name = NULL;
	int				  opt_driver_type = DRIVER_IR;
	const char		 *opt_device_name = NULL;
	const char		 *suffix;
	int				  opt_verbose = 0;
	int				  opt_hrm = 0;
	BUF				 *files;
	BUF				 *buf;
	workout_t		 *w;
	int				  offset;
	int				  opt_raw = 0;
	int				  count = 0;
	int				  ch;
	int				  ok;
	time_t			  ft;
	FILE*			  f;

	snprintf(inipath, PATH_MAX, "%s/.s725rc", getenv("HOME"));
	yyin = fopen(inipath, "r");
	if (yyin != NULL) {
		conf_filename = inipath;
		int ret = yyparse();
		if (ret != 0)
			exit(1);
		if (conf_driver_type != DRIVER_UNKNOWN)
			opt_driver_type = conf_driver_type;
		if (conf_device_name != NULL)
			opt_device_name = conf_device_name;
		if (conf_directory_name != NULL)
			opt_directory_name = conf_directory_name;
	}

	while ((ch = getopt(argc, argv, "d:D:f:hHrv")) != -1) {
		switch (ch) {
		case 'd':
			opt_driver_name = optarg;
			opt_driver_type = driver_name_to_type(opt_driver_name);
			if (opt_driver_type == DRIVER_UNKNOWN) {
				fprintf(stderr, "unknown driver type: %s\n", opt_driver_name);
				exit(1);
			}
			break;
		case 'D':
			opt_device_name = optarg;
			break;
		case 'f':
			opt_directory_name = optarg;
			break;
		case 'H':
			opt_hrm = 1;
			break;
		case 'r':
			opt_raw = 1;
			break;
		case 'v':
			opt_verbose++;
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

	if (opt_driver_type == DRIVER_IR || opt_driver_type == DRIVER_SERIAL) {
		if (!opt_device_name) {
			fprintf(stderr, "error: device name required for %s driver\n",
					driver_type_to_name(opt_driver_type));
			exit(1);
		}
	}
	
	if (opt_verbose) {
		fprintf(stderr, "driver name: %s\n", driver_type_to_name(opt_driver_type));
		fprintf(stderr, "driver type: %d\n", opt_driver_type);
		fprintf(stderr, "device name: %s\n", opt_device_name ? opt_device_name : "");
		fprintf(stderr, "directory name: %s\n", opt_directory_name ? opt_directory_name : "");
	}

	ok = driver_init (opt_driver_type , opt_device_name);
	if (ok != 1) {
		fprintf(stderr, "error: driver_init failed\n");
		exit(1);
	}

	if (opt_directory_name) {
		opt_directory_name = realpath(opt_directory_name, path);
	} else {
		getcwd(path, sizeof(path));
		opt_directory_name = path;
	}

	if (!opt_directory_name) {
		fprintf(stderr, "error: could not resolve path. check -f\n");
		exit(1);
	}

	if (driver_open() < 0) {
		fprintf(stderr,"error: unable to open port: %s\n", strerror(errno));
		exit(1);
	}

	files = buf_alloc(0);

	if (opt_raw)
		suffix = "srd";
	else
		if (opt_hrm)
			suffix = "hrm";
		else
			suffix = "txt";
	
	if (files_get(files)) {
		buf = buf_alloc(0);
		offset = 0;
		count = 0;
		while (files_split(files, &offset, buf)) {
			char tmbuf[128];
			char fnbuf[BUFSIZ];

			count++;
			ft = files_timestamp(buf, 0);
			strftime(tmbuf, sizeof(tmbuf), "%Y%m%dT%H%M%S", localtime(&ft));
			snprintf(fnbuf, sizeof(fnbuf), "%s/%s.%05zd.%s",
					 opt_directory_name, tmbuf, buf_len(buf), suffix);

			if (opt_raw) {
				f = fopen(fnbuf, "w");
				if (f) {
					fprintf(stderr, "File %02d: Saved as %s\n", count, fnbuf);
					fwrite(buf_get(buf), buf_len(buf), 1, f);
					fclose(f);
				} else {
					printf("File %02d: Unable to save %s: %s\n",
						   count,fnbuf,strerror(errno));
				}
			} else {
				w = workout_read_buf(buf);
				if (w) {
					f = fopen(fnbuf, "w");
					if (f) {
						fprintf(stderr, "File %02d: Saved as %s\n", count, fnbuf);
						if (opt_hrm)
							workout_print_hrm(w, f);
						else
							workout_print(w, f, S725_WORKOUT_FULL);
						fclose(f);
					} else {
						printf("File %02d: Unable to save %s: %s\n",
							   count,fnbuf,strerror(errno));
					}
					workout_free(w);
				} else {
					fprintf(stderr, "failed to parse workout for %s\n", fnbuf);
				}
			}
		}
		printf("Saved %d file%s\n", count, (count==1) ? "" : "s");
	}

	buf_free(files);
	driver_close();
	return 0;
}

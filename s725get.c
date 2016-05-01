
/*
 * Copyright (C) 2016  Ralf Horstmann

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
#include "log.h"
#include "misc.h"
#include "workout.h"
#include "workout_print.h"

static void write_hrm_data(BUF *files, const char *directory, int format_hrm, int format_raw);
static void listen_bytes();
static void write_bytes(const char *byte, int n);

static void
usage(void) {
	printf("usage: s725get [-hHrv] [-b byte] [-d driver] [-D device] [-f directory]\n");
	printf("        -b byte        send test byte (for debugging)\n");
	printf("        -n count       send n test bytes (for debugging)\n");
	printf("        -d driver      driver type: serial or stir. (default: serial).\n");
	printf("        -D device      device file. required for serial and ir driver.\n");
	printf("        -f directory   directory where output files are written to.\n");
	printf("                       default: current working directory\n");
	printf("        -l             listen for incoming data\n");
	printf("        -L             listen for incoming bytes (for debugging)\n");
	printf("        -t             get time\n");
	printf("        -H             write HRM format\n");
	printf("        -r             write raw srd format\n");
	printf("        -u             get user data\n");
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
	int				  opt_hrm = 0;
	BUF				 *files;
	int				  opt_raw = 0;
	int				  opt_time = 0;
	int				  opt_user = 0;
	int				  opt_listen = 0;
	int				  opt_listen_bytes = 0;
	const char		 *opt_byte = NULL;
	long long			  opt_count = 1;
	int				  ch;
	int				  ok;

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

	while ((ch = getopt(argc, argv, "b:d:D:f:hHlLn:rtuv")) != -1) {
		switch (ch) {
		case 'b':
			opt_byte = optarg;;
			break;
		case 'd':
			opt_driver_name = optarg;
			opt_driver_type = driver_name_to_type(opt_driver_name);
			if (opt_driver_type == DRIVER_UNKNOWN)
				fatalx("unknown driver type: %s", opt_driver_name);
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
		case 'l':
			opt_listen = 1;
			break;
		case 'L':
			opt_listen_bytes = 1;
			break;
		case 'n':
			opt_count = strtoll(optarg, NULL, 10);
			if (opt_count < 0 || opt_count > 100)
				opt_count = 1;
			break;
		case 'r':
			opt_raw = 1;
			break;
		case 't':
			opt_time = 1;
			break;
		case 'u':
			opt_user = 1;
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

	if (opt_driver_type == DRIVER_IR || opt_driver_type == DRIVER_SERIAL) {
		if (!opt_device_name)
			fatalx("device name required for %s driver",
				   driver_type_to_name(opt_driver_type));
	}
	
	log_info("driver name: %s", driver_type_to_name(opt_driver_type));
	log_info("driver type: %d", opt_driver_type);
	log_info("device name: %s", opt_device_name ? opt_device_name : "");
	log_info("directory name: %s", opt_directory_name ? opt_directory_name : "");

	ok = driver_init(opt_driver_type, opt_device_name);
	if (ok != 1)
		fatalx("driver_init failed");

	if (opt_directory_name) {
		opt_directory_name = realpath(opt_directory_name, path);
	} else {
		getcwd(path, sizeof(path));
		opt_directory_name = path;
	}

	if (!opt_directory_name)
		fatalx("could not resolve path. check -f");

	if (driver_open() < 0)
		fatalx("unable to open port: %s", strerror(errno));

	if (opt_byte) {
		write_bytes(opt_byte, opt_count);
		driver_close();
		return 0;
	}

	if (opt_listen_bytes) {
		listen_bytes();
		driver_close();
		return 0;
	}
	
	if (opt_time) {
		time_get();
		driver_close();
		return 0;
	}

	if (opt_user) {
		user_get();
		driver_close();
		return 0;
	}

	files = buf_alloc(0);

	if (opt_listen) {
		if (files_listen(files)) {
			write_hrm_data(files, opt_directory_name, opt_hrm, opt_raw);
		}
	} else {
		if (files_get(files)) {
			write_hrm_data(files, opt_directory_name, opt_hrm, opt_raw);
		}
	}

	buf_free(files);
	driver_close();
	return 0;
}

static void
listen_bytes()
{
	unsigned char byte;
	while (1) {
		if (driver_read_byte(&byte)) {
			log_write("0x%02hhx %c\n", byte, byte);
		}
	}
}

static void
write_bytes(const char *byte, int count)
{
	BUF	*buf;
	unsigned char mask = 128;
	unsigned char out;
	int c;
	int i;
	sscanf(byte, "%u", &c);
	out = (unsigned char) c;
	buf = buf_alloc(0);
	for (i = 0; i < count; ++i)
		buf_putc(buf, out);
	if (log_get_level() > 0) {
		log_writeln("byte: %hhu", out);
		log_write("bits: ");
		for (; mask; mask >>=1) {
			log_write("%u", out & mask ? 1 : 0);
		}
		log_writeln("");
	}
	driver_write(buf);
	buf_free(buf);
}

static void
write_hrm_data(BUF *files, const char* directory, int format_hrm, int format_raw)
{
	const char *suffix;
	workout_t *w;
	FILE* f;
	BUF *buf;
	time_t ft;
	int	offset;
	int count;

	if (format_raw)
		suffix = "srd";
	else
		if (format_hrm)
			suffix = "hrm";
		else
			suffix = "txt";

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
				 directory, tmbuf, buf_len(buf), suffix);

		if (format_raw) {
			f = fopen(fnbuf, "w");
			if (f) {
				log_writeln("File %02d: Saved as %s", count, fnbuf);
				fwrite(buf_get(buf), buf_len(buf), 1, f);
				fclose(f);
			} else {
				log_writeln("File %02d: Unable to save %s: %s",
							count, fnbuf, strerror(errno));
			}
		} else {
			w = workout_read_buf(buf);
			if (w) {
				f = fopen(fnbuf, "w");
				if (f) {
					log_writeln("File %02d: Saved as %s", count, fnbuf);
					if (format_hrm)
						workout_print_hrm(w, f);
					else
						workout_print_txt(w, f, S725_WORKOUT_FULL);
					fclose(f);
				} else {
					log_writeln("File %02d: Unable to save %s: %s",
								count, fnbuf, strerror(errno));
				}
				workout_free(w);
			} else {
				log_writeln("Failed to parse workout for %s", fnbuf);
			}
		}
	}
	buf_free(buf);
	log_writeln("Saved %d file%s", count, (count==1) ? "" : "s");
}

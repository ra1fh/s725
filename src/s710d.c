/* $Id: s710d.c,v 1.3 2002/10/10 10:11:13 dave Exp $ */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "s710.h"

/* globals */

static volatile sig_atomic_t gSigFlag;
static unsigned int          gDaemonize = 1;
static unsigned int          gVerbose   = 0;

/* function declarations */

static void          s710d_signal_handler ( int signum );
static void          s710d_log            (u_int level, const char *fmt, ...);
static void          s710d_usage          (void);

/* main program */
int
main ( int argc, char **argv )
{
	char			path[PATH_MAX];
	const char	   *filedir = NULL;
	files_t			file;
	S710_Driver		driver;
	const char	   *driver_name = NULL;
	const char	   *device = NULL;
	int				ch;

	while ( (ch = getopt(argc,argv,"d:f:hnv")) != -1 ) {
		switch (ch) {
		case 'd':
			driver_name = optarg;
			break;
		case 'f':
			filedir = optarg;
			break;
		case 'h':
			s710d_usage();
			exit(0);
			break;
		case 'n':
			gDaemonize = 0;
			break;
		case 'v':
			gVerbose++;
			break;
		default:
			s710d_usage();
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;
	device = argv[0];

	if (driver_init(driver_name, device, &driver) != 1) {
		s710d_usage();
		exit(1);
	}

	if (gDaemonize) {
		daemon(0, 0);
		openlog("s710d", LOG_PID | LOG_NDELAY, LOG_USER);
	}

	if (filedir) {
		filedir = realpath(filedir, path);
	} else if ((filedir = getenv("S710_FILEDIR")) != NULL) {
		filedir = realpath(filedir,path);
	} else {
		filedir = S710_FILEDIR;
	}

	if (!filedir) {
		s710d_log(0, "could not resolve path. check S710_FILEDIR or -f\n");
		exit(1);
	}

	s710d_log(0,"started.\n");

	if ( driver_open ( &driver, S710_MODE_RDONLY ) < 0 ) {
		s710d_log(0, "failed to initialize, aborting\n");
		exit(1);
	}

	/* install signal handlers */
	signal(SIGINT, s710d_signal_handler);
	signal(SIGTERM, s710d_signal_handler);

	/* enter infinite loop */
	while (!gSigFlag ) {
		if (receive_file(&driver,&file, s710d_log)) {
			save_files(&file,filedir, s710d_log);
			print_files(&file, s710d_log); 
		}	   
		usleep(10000);
	}

	driver_close ( &driver );

	/* note the signal */
	s710d_log(0, "ended: received signal %d\n",gSigFlag);

	if (gDaemonize) {
		closelog();	  /* optional */
	}

	return 0;
}

static void
s710d_usage()
{
	printf("usage: s710d [-h] [-n] [-d driver] [-f filedir] [device file]\n");
	printf("	   -d driver	may be either serial, ir, or usb\n");
	printf("	   -f filedir	is the directory where output files are written to\n");
	printf("	   -n			do not fork and log to stdout\n");
	printf("	   device file	is required for serial and ir drivers\n");
	printf("					alternative is S710_FILEDIR environment variable\n");
}

static void
s710d_log(u_int level, const char *fmt, ...)
{
	va_list ap;

	if (level > gVerbose)
		return;

	va_start(ap, fmt);
	if (gDaemonize) {
		vsyslog(LOG_INFO, fmt, ap);
	} else {
		vfprintf(stderr, fmt, ap);
		fflush(stderr);
	}
	va_end(ap);
}

static void
s710d_signal_handler ( int signum )
{
	gSigFlag = signum;
}

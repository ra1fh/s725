/* $Id: s710d.c,v 1.3 2002/10/10 10:11:13 dave Exp $ */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "s710.h"
#include "config.h"

/* globals */

static int           gSigFlag;

/* function declarations */

static void          signal_handler   ( int signum );

/* usage */

void usage() {
    printf("usage: s710d [-h] [-d driver] [-f filedir] [device file]\n");
	printf("       driver       may be either serial, ir, or usb\n");
	printf("       device file  is required for serial and ir drivers.\n");
	printf("       filedir      is the directory where output files are written to.\n");
	printf("                    alternative is S710_FILEDIR environment variable.\n");
}

/* main program */
int
main ( int argc, char **argv )
{
  pid_t           pid;
  char            path[PATH_MAX];
  char           *filedir = NULL;
  files_t         file;
  S710_Driver     driver;
  int             ok;
  const char     *driver_name = NULL;
  const char     *device = NULL;
  int             ch;

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
		  exit(0);
		  break;
	  default:
		  usage();
		  exit(1);
	  }
  }
  argc -= optind;
  argv += optind;
  device = argv[0];

  ok = driver_init (driver_name , device, &driver );

  if ( ok != 1 ) {
	  usage();
	  exit(1);
  }

  if (filedir) {
	  filedir = realpath(filedir, path);
  } else if ((filedir = getenv("S710_FILEDIR")) != NULL) {
	  filedir = realpath(filedir,path);
  } else {
	  filedir = S710_FILEDIR;
  }

  if (!filedir) {
	  printf("could not resolve path. check S710_FILEDIR or -f\n");
	  exit(1);
  }

  /* fork */

  if ( (pid = fork()) == 0 ) {

    /* print syslog messages to stderr as well (debugging) */

    openlog("s710d",LOG_CONS|LOG_PID|LOG_PERROR,LOG_USER);
    syslog(LOG_NOTICE,"started");

    if ( driver_open ( &driver, S710_MODE_RDONLY ) < 0 ) {
      syslog(LOG_CRIT,"failed to initialize, aborting");
      exit(1);
    }

    /* install signal handlers */

    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);

    /* enter infinite loop */
    
    while ( !gSigFlag ) {

      if ( receive_file(&driver,&file,stderr) ) {
	save_files(&file,filedir,stderr);
	print_files(&file,stderr); 
      }      

      usleep(10000);
    }

    driver_close ( &driver );

    /* note the signal */

    syslog(LOG_NOTICE,"ended: received signal %d",gSigFlag);
    closelog();   /* optional */
  }

  return 0;
}



/* signal handler just sets a global flag */

static void
signal_handler ( int signum )
{
  gSigFlag = signum;
}

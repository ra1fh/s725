/* $Id: s710sh.c,v 1.7 2004/09/21 08:16:05 dave Exp $ */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "s710.h"

static volatile int quit;

static void     usage(void);
static void     explain_command	(int c, attribute_map_t *m);
#if 0
static char   **s710sh_argextra(char **argv, int argc, int off, struct cmd *cmdp);
#endif
static int      s710sh_checkarg(char *arg);
static int      s710sh_checkcmd(int argc, char **argv, int *off, struct cmd *cmd);
static char    *s710sh_completion(const char *str, int state);
static int      s710sh_help(struct cmd *c, char **argv);
static void     s710sh_log(unsigned int level, const char *fmt, ...);
static ssize_t  s710sh_strip(char *str);
static char   **s710sh_arg2argv(char *arg, int *argc);

static struct cmd cmds[] = CMDS;

static void hexdump(void *ptr, int buflen) { 
	unsigned char *buf = (unsigned char*)ptr; 
	int i, j; 
	for (i=0; i<buflen; i+=16) { 
		fprintf(stderr, "%06x: ", i); 
		for (j=0; j<16; j++)  
			if (i+j < buflen) 
				fprintf(stderr, "%02x ", buf[i+j]); 
			else 
				fprintf(stderr, "   "); 
		fprintf(stderr, " "); 
		for (j=0; j<16; j++)  
			if (i+j < buflen) 
				fprintf(stderr, "%c", isprint(buf[i+j]) ? buf[i+j] : '.'); 
		fprintf(stderr, "\n"); 
	} 
} 

static void
usage(void) {
	printf("usage: s710sh [-h] [-d driver] [-f filedir] [device file]\n");
	printf("	   driver		may be either serial, ir, or usb\n");
	printf("	   device file	is required for serial and ir drivers.\n");
	printf("	   filedir		is the directory where output files are written to.\n");
	printf("					alternative is S710_FILEDIR environment variable.\n");
}

static int
s710sh_help(struct cmd *c, char **argv) {
	u_int i;

	printf("valid commands:\n");
	for (i = 0; c[i].name != NULL; i++) {
		printf("  %s", c[i].name);
		if (c[i].minargs > 0)
			printf(" { arg }");
		else if (c[i].maxargs > 0)
			printf(" [ arg ]");
		printf("\n");
	}
	return (0);
}

int
s710sh_checkarg(char *arg)
{
	size_t len;
	u_int i;

	if (!(len = strlen(arg)))
		return (0);

#define allowed_in_string(_x)                                           \
	((isalnum(_x) || isprint(_x)) &&				\
	(_x != '%' && _x != '\\' && _x != ';' && _x != '&' && _x != '|'))

	for (i = 0; i < len; i++) {
		if (!allowed_in_string(arg[i])) {
			fprintf(stderr, "invalid character in input\n");
			return (EPERM);
		}
	}

	return (0);
}

static char *
s710sh_completion(const char *str, int state) {
	static int s710sh_complidx, len;
	const char *name;

#ifdef DEBUG
	printf("completion entry: > text=%s state=%d idx=%d\n", str, state, s710sh_complidx);
#endif

	if (state == 0) {
		len = strlen(str);
		s710sh_complidx = 0;
	}
	while ((name = cmds[s710sh_complidx].name) != NULL) {
		s710sh_complidx++;
		if (strncmp(name, str, len) == 0) {
#ifdef DEBUG
			printf("completion entry: < %s\n", name);
#endif
			return (strdup(name));
		}
	}

#ifdef DEBUG
	printf("completion entry: < NULL\n");
#endif
	return (NULL);
}

int
s710sh_checkcmd(int argc, char **argv, int *off, struct cmd *cmd)
{
	char **cmdp = NULL, *cmdstr = NULL;
	int i, ncmd, v, ret = -1;

	if ((cmdstr = strdup(cmd->name)) == NULL)
		goto done;
	if ((cmdp = s710sh_arg2argv(cmdstr, &ncmd)) == NULL)
		goto done;
	if (ncmd > argc || argc > (ncmd + cmd->maxargs))
		goto done;

	for (i = 0; i < ncmd; i++)
		if (strcmp(argv[i], cmdp[i]) != 0)
			goto done;

	if ((v = argc - ncmd) < 0 ||
	    (*off != -1 && *off < v))
		goto done;
	if (cmd->minargs && v < cmd->minargs) {
		ret = EINVAL;
		goto done;
	}
	*off = v;
	ret = 0;

 done:
	if (cmdp != NULL)
		free(cmdp);
	if (cmdstr != NULL)
		free(cmdstr);
	return (ret);
}

char **
s710sh_arg2argv(char *arg, int *argc)
{
	char **argv, *ptr = arg;
	size_t len;
	u_int i, c = 1;

	if (s710sh_checkarg(arg) != 0)
		return (NULL);
	if (!(len = strlen(arg)))
		return (NULL);

	/* Count elements */
	for (i = 0; i < len; i++) {
		if (isspace(arg[i])) {
			/* filter out additional options */
			if (arg[i + 1] == '-') {
				printf("invalid input\n");
				return (NULL);
			}
			arg[i] = '\0';
			c++;
		}
	}
	if (arg[0] == '\0')
		return (NULL);

	/* Generate array */
	if ((argv = calloc(c + 1, sizeof(char *))) == NULL) {
		printf("fatal error: %s\n", strerror(errno));
		return (NULL);
	}

	argv[c] = NULL;
	*argc = c;

	/* Fill array */
	for (i = c = 0; i < len; i++) {
		if (arg[i] == '\0' || i == 0) {
			if (i != 0)
				ptr = &arg[i + 1];
			argv[c++] = ptr;
		}
	}

	return (argv);
}

#if 0
char **
s710sh_argextra(char **argv, int argc, int off, struct cmd *cmdp)
{
	char **new_argv;
	int i, c = 0, n;

	if ((n = argc - off) < 0)
		return (NULL);

	/* Count elements */
	for (i = 0; cmdp->earg[i] != NULL; i++)
		c++;

	/* Generate array */
	if ((new_argv = calloc(c + n + 1, sizeof(char *))) == NULL) {
		printf("fatal error: %s\n", strerror(errno));
		return (NULL);
	}

	/* Fill array */
	for (i = c = 0; cmdp->earg[i] != NULL; i++)
		new_argv[c++] = cmdp->earg[i];

	/* Append old array */
	for (i = n; i < argc; i++)
		new_argv[c++] = argv[i];

	new_argv[c] = NULL;

	if (argv != NULL)
		free(argv);

	return (new_argv);
}
#endif 

ssize_t
s710sh_strip(char *str)
{
	size_t len;

	if ((len = strlen(str)) < 1)
		return (0);

	if (isspace(str[len - 1])) {
		str[len - 1] = '\0';
		return (s710sh_strip(str));
	}

	return (strlen(str));
}

int
main(int argc, char **argv)
{
	int				  i;
	int				  j;
	char			  request[BUFSIZ];
	packet_t		 *p = NULL;
	int				  go;
	const char		 *filedir = NULL;
	char			 *rend;
	char			 *rbeg;
	char			  path[PATH_MAX];
	int				  ok;
	S710_Driver		  d;
	attribute_map_t	  map[S710_MAP_TYPE_COUNT];
	struct tm		  now;
	time_t			  t;
	static int		  reset;
	const char		 *driver_name = NULL;
	const char		 *device = NULL;
	int				  ch;

	struct cmd *cmd = NULL;
	int ncmd, ret, v = -1;
	char *line, **argp = NULL;

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

	ok = driver_init (driver_name , device, &d );

	if ( ok != 1 ) {
		printf("problem with driver_init\n");
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

	if ( driver_open ( &d, S710_MODE_RDWR ) < 0 ) {
		fprintf(stderr,"unable to open port: %s\n",strerror(errno));
		exit(1);
	}

	memset(map,0,sizeof(map));

	load_user_attributes(&map[S710_MAP_TYPE_USER]);
	load_watch_attributes(&map[S710_MAP_TYPE_WATCH]);
	load_logo_attributes(&map[S710_MAP_TYPE_LOGO]);
	load_bike_attributes(&map[S710_MAP_TYPE_BIKE]);
	load_exercise_attributes(&map[S710_MAP_TYPE_EXERCISE]);
	load_reminder_attributes(&map[S710_MAP_TYPE_REMINDER]);

	setvbuf(stdout,NULL,_IONBF,0);

	rl_readline_name = "s710sh";
	rl_completion_entry_function = s710sh_completion;

	/* Ignore the whitespace character */
	/* the default is (space!!!):   " \t\n\"\\'`@$><=;|&{(" */
	rl_basic_word_break_characters = "\t\n\"\\'`@$><=;|&{(";
	while (!quit) {
		v = -1;

		if ((line = readline("s710sh> ")) == NULL) {
			printf("\n");
			quit = 1;
			goto next;
		}
		if (!s710sh_strip(line))
			goto next;
		if ((strcmp(line, "exit") == 0) || (strcmp(line, "quit") == 0)) {
			quit = 1;
			goto next;
		}

		add_history(line);

		if ((argp = s710sh_arg2argv(line, &ncmd)) == NULL) {
			printf("arg2argv return NULL\n");
			goto next;
		}

		for (i = 0; cmds[i].name != NULL; i++) {
			ret = s710sh_checkcmd(ncmd, argp, &v, &cmds[i]);
			if (ret == 0)
				cmd = &cmds[i];
			else if (ret == EINVAL) {
				printf("invalid number of arguments\n");
				goto next;
			}
		}

		printf("cmd: %d\n", (int) cmd);
		if (cmd) {
			printf("func: %d\n", (int) cmd->func);
			printf("name: %s\n", cmd->name);
		}
		if (cmd == NULL) {
			printf("invalid command\n");
		} else if (cmd->func != NULL) {
			cmd->func(cmds, argp);
		} else if (cmd->packet_type >= S710_GET_OVERVIEW &&
				   cmd->packet_type <= S710_CLOSE_CONNECTION) {
			handle_retrieval(&d, cmd->packet_type, filedir, s710sh_log);
		}
/*
		} else {
			if ((argp = lg_argextra(argp, ncmd, v, cmd)) == NULL)
				goto next;
			lg_exec(cmd->earg[0], argp);
		}
*/

 next:
		if (argp != NULL) {
			free(argp);
			argp = NULL;
		}
		if (line != NULL) {
			free(line);
			line = NULL;
		}
		cmd = NULL;
	}

	driver_close(&d);
	for ( i = 0; i < S710_MAP_TYPE_COUNT; i++ ) {
		clear_attribute_map(&map[i]);
	}
	exit(0);

	if ( driver_open ( &d, S710_MODE_RDWR ) != -1 ) {

		/* initialize */

		memset(map,0,sizeof(map));

		load_user_attributes(&map[S710_MAP_TYPE_USER]);
		load_watch_attributes(&map[S710_MAP_TYPE_WATCH]);
		load_logo_attributes(&map[S710_MAP_TYPE_LOGO]);
		load_bike_attributes(&map[S710_MAP_TYPE_BIKE]);
		load_exercise_attributes(&map[S710_MAP_TYPE_EXERCISE]);
		load_reminder_attributes(&map[S710_MAP_TYPE_REMINDER]);

		printf("\ns710sh> ");
		
		/* main loop */

		while ( 1 ) {

			if (fgets(request,sizeof(request),stdin) == NULL) {
				if (feof(stdin)) {
					printf("\nExit\n");
					break;
				} else {
					printf("\nError\n");
					break;
				}
			}
			if ( request == NULL || *request == '\n' ) {
				printf("s710sh> ");
				continue;
			}

			if ( is_like(request,"quit") ||
				 is_like(request,"exit") ) {

				/* quit */

				printf("\nExit\n");

				break;
			} else if ( is_like(request,"help") ||
						is_like(request,"????") ) {

				/* print help */

				printf("\noptions:\n\n");
				for ( i = 0; i < num_packets(); i++ ) {
					if ( i == S710_CONTINUE_TRANSFER ) continue;
					p = packet(i);
					printf("%s\n",p->name);
				}
				printf("synchronize time\n");
				printf("\n");
			}

			/* reset */

			go = 0;

			/* loop through available packets */

			for ( i = 0; i < num_packets(); i++ ) {
	
				/* we don't let people send 'continue transfer' packets directly */

				if ( i == S710_CONTINUE_TRANSFER ) continue;

				/* is the user trying to send this packet?	if so, break. */
	
				p = packet(i);
				if ( is_like(request,p->name) ) {
					go = 1;
					break;
				}
			}

			/* 
			   if we've got a packet, deal with it.	 note: this could be done
			   much more elegantly, but i don't have time right now.
			*/

			if ( go ) {
				if ( i >= S710_GET_OVERVIEW && i <= S710_CLOSE_CONNECTION ) {
					handle_retrieval(&d, i, filedir, s710sh_log);
				} else if ( i >= S710_SET_USER && i <= S710_SET_REMINDER_7 ) {

					/* this is a transfer request.	we need to make sure we've 
					   seen the '{' character on the FIRST line.  we then need
					   to read as many lines as it takes until we get one with
					   a '}' character.	 then we can process the request. */

					rbeg = &request[0];
					rend = strchr(request,'{');
					if ( rend != NULL ) {

						/* we've got the opening brace.	 read until we get the 
						   closing brace.  sorry, no syntax checking. */

						while ( !strchr(rend,'}') ) {
							rend = strchr(rend,'\n');
							fgets(rend,sizeof(request)-(rend-rbeg+1),stdin);
						}
		
						/* ok, now we're ready to go. */

						handle_transfer(&d,i,request,map);

					} else {
						printf("\nSyntax error: opening brace '{' required.\n\n");		
						explain_command(i,map);
					}
				} else if ( i == S710_HARD_RESET ) { 

					/* don't let people hard reset on the first try. */

					if ( reset < 2 ) {
						printf("\nAre you ");
						for ( j = 0; j < reset; j++ ) {
							printf("REALLY ");
						}
						printf("sure you want do reset to factory defaults?!\n");
						printf("If so, type \"%s\" %d more time%s\n\n",p->name,
							   2-reset,(reset==1)?"":"s");
						reset++;
					} else {
						printf("\nOK, you asked for it...\n\n");
						send_packet(p,&d);
						reset = 0;
					}
				}
			} else {

				/* a special command. */

				if ( is_like(request,"synchronize time") ) {
					t = time(NULL);
					localtime_r(&t,&now);
					snprintf(request,
							 sizeof(request),
							 "set watch { "
							 "time1.tm_year = %d, "
							 "time1.tm_mon = %d, "
							 "time1.tm_mday = %d, "
							 "time1.tm_hour = %d, "
							 "time1.tm_min = %d, "
							 "time1.tm_sec = %d, "
							 "time1.tm_wday = %d, "
							 "which_time = 1 }\n",
							 now.tm_year + 1900,
							 now.tm_mon + 1,
							 now.tm_mday,
							 now.tm_hour,
							 now.tm_min,		   
							 now.tm_sec,
							 now.tm_wday + 1);
					handle_transfer(&d,S710_SET_WATCH,request,map);
				}
			}

			printf("s710sh> ");
		}

		driver_close(&d);
		for ( i = 0; i < S710_MAP_TYPE_COUNT; i++ ) {
			clear_attribute_map(&map[i]);
		}

	} else {
		fprintf(stderr,"unable to open port: %s\n",strerror(errno));
	}

	return 0;
}


/* print syntax for "set" commands */

static void
explain_command ( int c, attribute_map_t *m )
{
	static int		 general;

	/* this is kind of stupid, but i figure the user doesn't need to see
	   this portion of the instructions more than a few times before they
	   'get it'. */

	if ( general < 3 ) {
		printf("\"Set\" commands are of the following form:\n\n"
			   "set <property> { attribute-value-list }\n\n"
			   "where attribute-value-list is a comma-separated list of\n"
			   "attribute-value pairs, and an attribute-value pair is of\n"
			   "the form\n\n"
			   "attribute = value\n\n"
			   "Attribute names depend on the property being set.  Values\n"
			   "are either strings enclosed in double quotation marks,\n"
			   "integer values, or boolean values.	Boolean values may be\n"
			   "represented by 1, 0, \"true\", or \"false\".  Integer\n"
			   "values are usually restricted to be within a certain range;\n"
			   "where this is the case, the range is given.	 String values\n"
			   "are also sometimes restricted.\n\n"
			   "It is not necessary to provide the entire list of attribute-\n"
			   "value pairs if you wish to set a property.	The attributes\n"
			   "which you do not specify will not be changed.\n\n"
			   "You can shortcut attribute names in the same way that you can\n"
			   "shortcut commands: any unique abbreviation of the attribute\n"
			   "will be expanded to match.\n\n");
		printf("Example command strings:\n\n"
			   "set user { name = \"Dave\", gender = \"male\" }\n"
			   "set user { units = \"metric\", weight = 84, height = 190 }\n"
			   "set user { altimeter = 1, energy_expenditure = 1 }\n\n");
		printf("Example command strings, maximally abbreviated:\n\n"
			   "s u { n = \"Dave\", g = \"male\" }\n"
			   "s u { un = \"metric\", w = 84, h = 190 }\n"
			   "s u { al = 1, e = 1 }\n\n");
		general++;
	}

	/* now for the specific instructions */

	switch ( c ) {
	case S710_SET_USER:
		print_attribute_map(c,&m[S710_MAP_TYPE_USER]);
		break;
	case S710_SET_WATCH:
		print_attribute_map(c,&m[S710_MAP_TYPE_WATCH]);
		break;
	case S710_SET_LOGO:
		print_attribute_map(c,&m[S710_MAP_TYPE_LOGO]);
		break;
	case S710_SET_BIKE:
		print_attribute_map(c,&m[S710_MAP_TYPE_BIKE]);
		break;
	case S710_SET_EXERCISE_1:
	case S710_SET_EXERCISE_2:
	case S710_SET_EXERCISE_3:
	case S710_SET_EXERCISE_4:
	case S710_SET_EXERCISE_5:
		print_attribute_map(c,&m[S710_MAP_TYPE_EXERCISE]);
		break;
	case S710_SET_REMINDER_1:
	case S710_SET_REMINDER_2:
	case S710_SET_REMINDER_3:
	case S710_SET_REMINDER_4:
	case S710_SET_REMINDER_5:
	case S710_SET_REMINDER_6:
	case S710_SET_REMINDER_7:
		print_attribute_map(c,&m[S710_MAP_TYPE_REMINDER]);
		break;
	default:
		break;
	}
}

static void
s710sh_log(u_int level, const char *fmt, ...)
{
	va_list ap;

#if 0
	if (level > gVerbose)
		return;
#endif
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
}

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
static int      s710sh_checkarg(char *arg);
static int      s710sh_checkcmd(int argc, char **argv, int *off, struct cmd *cmd);
static char    *s710sh_completion(const char *str, int state);
static int      s710sh_help(struct cmd *c, char **argv);
static void     s710sh_log(unsigned int level, const char *fmt, ...);
static ssize_t  s710sh_strip(char *str);
static char   **s710sh_arg2argv(char *arg, int *argc);

static struct cmd cmds[] = CMDS;

static void
usage(void) {
	printf("usage: s710sh [-h] [-d driver] [-f filedir] [device file]\n");
	printf("       driver       may be either serial, ir, or usb\n");
	printf("       device file  is required for serial and ir drivers.\n");
	printf("       filedir      is the directory where output files are written to.\n");
	printf("                    alternative is S710_FILEDIR environment variable.\n");
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
	const char		 *filedir = NULL;
	char			  path[PATH_MAX];
	int				  ok;
	S710_Driver		  d;
	attribute_map_t	  map[S710_MAP_TYPE_COUNT];
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

	ok = driver_init (driver_name , device, &d );

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
		filedir = S710_FILEDIR;
	}

	if (!filedir) {
		printf("could not resolve path. check S710_FILEDIR or -f\n");
		return 1;
	}

	if ( driver_open ( &d, S710_MODE_RDWR ) < 0 ) {
		fprintf(stderr,"unable to open port: %s\n",strerror(errno));
		return 1;
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
	return 0;
}

static void
s710sh_log(u_int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
}

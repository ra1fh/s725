
%token TOKDEVICE
%token TOKDIRECTORY
%token TOKDRIVER
%token TOKSERIAL
%token TOKUSB
%token TOKIR
%token TOKSTIR
%token EOL

%token <sval> STRING
%type  <ival> driver_type

%union {
	char *sval;
	int   ival;
}
	
%{

#include <stdio.h>
#include <string.h>

#include "driver.h"

void yyerror (char const *s);
extern int yylineno;

int conf_driver_type = DRIVER_UNKNOWN;
char *conf_device_name = NULL;
char *conf_directory_name = NULL;
const char *conf_filename = NULL;
	
%}

%%

commands:       /* empty */
		|		commands EOL
		| 		commands command EOL
				;

command: 		device
		| 		driver
		|		directory
				;

device:			TOKDEVICE '=' STRING
				{
					conf_device_name = strdup($3 + 1);
					conf_device_name[strlen(conf_device_name) - 1] = 0;
				}
				;

directory:		TOKDIRECTORY '=' STRING
				{
					conf_directory_name = strdup($3 + 1);
					conf_directory_name[strlen(conf_directory_name) - 1] = 0;
				}
				;

driver:			TOKDRIVER '=' driver_type
				{
					conf_driver_type = $3;
				}
				;

driver_type: 	TOKIR     { $$ = DRIVER_IR;     }
		| 		TOKUSB    { $$ = DRIVER_USB;    }
		| 		TOKSERIAL { $$ = DRIVER_SERIAL; }
		| 		TOKSTIR   { $$ = DRIVER_STIR; }
				;


%%

void
yyerror (char const *s)
{
	fprintf (stderr, "%s: line %d: %s\n", conf_filename, yylineno, s);
}

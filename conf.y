
%token TOKDEVICE
%token TOKDIRECTORY
%token TOKDRIVER
%token TOKSERIAL
%token TOKIRDA
%token TOKSTIR
%token TOKFORMAT
%token TOKHRM
%token TOKRAW
%token TOKTCX
%token TOKTXT
%token EOL

%token <sval> STRING
%type  <ival> driver_type
%type  <ival> format_type

%union {
	char *sval;
	int   ival;
}
	
%{

#include <stdio.h>
#include <string.h>

#include "driver.h"
#include "format.h"

void yyerror (char const *s);
int yylex();
extern int yylineno;

int conf_driver_type = DRIVER_UNKNOWN;
int conf_format_type = FORMAT_UNKNOWN;
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
		|		format
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

driver_type:	TOKSERIAL { $$ = DRIVER_SERIAL; }
		| 		TOKSTIR   { $$ = DRIVER_STIR; }
		|		TOKIRDA   { $$ = DRIVER_IRDA; }
				;

format:			TOKFORMAT '=' format_type
				{
					conf_format_type = $3;
				}
				;

format_type:	TOKHRM { $$ = FORMAT_HRM; }
		|		TOKRAW { $$ = FORMAT_RAW; }
		|		TOKTCX { $$ = FORMAT_TCX; }
		|		TOKTXT { $$ = FORMAT_TXT; }
				;

%%

void
yyerror (char const *s)
{
	fprintf (stderr, "%s: line %d: %s\n", conf_filename, yylineno, s);
}

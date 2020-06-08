/* conf.y - configuration file parser */

/*
 * Copyright (C) 2016  Ralf Horstmann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

%token TOKDEVICE
%token TOKDIRECTORY
%token TOKDRIVER
%token TOKSERIAL
%token TOKFORMAT
%token TOKHRM
%token TOKSRD
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
				;

format:			TOKFORMAT '=' format_type
				{
					conf_format_type = $3;
				}
				;

format_type:	TOKHRM { $$ = FORMAT_HRM; }
		|		TOKSRD { $$ = FORMAT_SRD; }
		|		TOKTCX { $$ = FORMAT_TCX; }
		|		TOKTXT { $$ = FORMAT_TXT; }
				;

%%

void
yyerror (char const *s)
{
	fprintf (stderr, "%s: line %d: %s\n", conf_filename, yylineno, s);
}

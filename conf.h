/* conf.h - configuration file parser */

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

#ifndef CONF_H
#define CONF_H

#include <stdio.h>

extern int conf_driver_type;
extern int conf_format_type;
extern const char *conf_device_name;
extern const char *conf_directory_name;
extern const char *conf_filename;
extern FILE *yyin;

int	yyparse(void);

#endif

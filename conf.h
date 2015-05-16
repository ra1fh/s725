
#ifndef CONF_H
#define CONF_H

#include <stdio.h>

extern int conf_driver_type;
extern const char *conf_device_name;
extern const char *conf_directory_name;
extern const char *conf_filename;
extern FILE *yyin;

int	yyparse(void);

#endif

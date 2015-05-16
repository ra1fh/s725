/*
 * public driver interface
 */

#ifndef DRIVER_H
#define DRIVER_H

#include "buf.h"

enum {
	DRIVER_UNKNOWN = 0,
	DRIVER_SERIAL,
	DRIVER_USB,
	DRIVER_IR
};

int			 driver_init(int driver_type, const char *device);
int			 driver_open();
int			 driver_write(BUF *buf);
int			 driver_read_byte(unsigned char *b);
int			 driver_close();
int			 driver_name_to_type(const char *driver_name);
const char*  driver_type_to_name(int driver_type);

#endif	/* DRIVER_H */

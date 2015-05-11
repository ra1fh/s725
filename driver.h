/*
 * public driver interface
 */

#ifndef DRIVER_H
#define DRIVER_H

#include "buf.h"

int		 driver_init(const char *driver_name, const char *device);
int		 driver_open();
int		 driver_write(BUF *buf);
int		 driver_read_byte(unsigned char *b);
int		 driver_close();

#endif	/* DRIVER_H */

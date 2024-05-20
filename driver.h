/* driver.h - public driver interface */

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

#ifndef DRIVER_H
#define DRIVER_H

#include "buf.h"

enum {
	DRIVER_UNKNOWN = 0,
	DRIVER_SERIAL,
};

int driver_init(int driver_type, const char *device);
int driver_open(void);
int driver_write(BUF *buf);
int driver_read(BUF *buf);
int driver_read_byte(unsigned char *b);
int driver_close(void);
int driver_uses_frames(void);
int driver_name_to_type(const char *driver_name);
const char* driver_type_to_name(int driver_type);

#endif	/* DRIVER_H */

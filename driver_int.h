/* driver_int.h - internal driver interface */

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

#ifndef DRIVER_INT_H
#define DRIVER_INT_H

#include <limits.h>

#include "buf.h"

struct s725_driver_ops;

struct s725_driver {
	struct s725_driver_ops *dops;
	void *data;
	int uses_frames;
	char path[PATH_MAX];
};

struct s725_driver_ops {
	int (*init)      (struct s725_driver* d);
	int (*read)      (struct s725_driver* d, BUF *buf);
	int (*read_byte) (struct s725_driver* d, unsigned char *byte);
	int (*write)     (struct s725_driver* d, BUF *buf);
	int (*close)     (struct s725_driver* d);
};

#endif	/* DRIVER_H */

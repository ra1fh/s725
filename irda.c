/* irda driver */

/*
 * Copyright (C) 2016 Ralf Horstmann
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

#include "driver_int.h"
#include "log.h"
#include "xmalloc.h"

static int irda_init(struct s725_driver *d);
static int irda_write(struct s725_driver *d, BUF *buf);
static int irda_read_byte(struct s725_driver *d, unsigned char *byte);
static int irda_close(struct s725_driver *d);

struct s725_driver_ops irda_driver_ops = {
	.init = irda_init,
	.read = irda_read_byte,
	.write = irda_write,
	.close = irda_close,
};

struct driver_private {
	int fd;
};

#define DP(x) ((struct driver_private *)x->data)


/* 
 * initialize the irda port
 */
static int  
irda_init(struct s725_driver *d)
{
	return 0;
}

static int
irda_close(struct s725_driver *d)
{
	return 0;
}
	
static int
irda_read_byte(struct s725_driver *d, unsigned char *byte)
{
	return 0;
}

static int
irda_write(struct s725_driver *d, BUF *buf)
{
	return 0;
}


/*
 * internal driver interface
 */

#ifndef DRIVER_INT_H
#define DRIVER_INT_H

#include <limits.h>

#include "buf.h"

struct s710_driver_ops;

struct s710_driver {
	char                    path[PATH_MAX];
	void                   *data;
	struct s710_driver_ops *dops;
};

struct s710_driver_ops {
	int (*init)  (struct s710_driver* d);
	int (*read)  (struct s710_driver* d, unsigned char *byte);
	int (*write) (struct s710_driver* d, BUF *buf);
	int (*close) (struct s710_driver* d);
};

#endif	/* DRIVER_H */

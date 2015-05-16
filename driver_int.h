/*
 * internal driver interface
 */

#ifndef DRIVER_INT_H
#define DRIVER_INT_H

#include <limits.h>

#include "buf.h"

struct s725_driver_ops;

struct s725_driver {
	char					path[PATH_MAX];
	void				   *data;
	struct s725_driver_ops *dops;
};

struct s725_driver_ops {
	int (*init)  (struct s725_driver* d);
	int (*read)  (struct s725_driver* d, unsigned char *byte);
	int (*write) (struct s725_driver* d, BUF *buf);
	int (*close) (struct s725_driver* d);
};

#endif	/* DRIVER_H */

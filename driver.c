/* driver.c - public driver interface */

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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver.h"
#include "driver_int.h"

extern struct s725_driver_ops serial_driver_ops;

static struct s725_driver *driver;

int
driver_init(const int driver_type, const char *device)
{
	if (driver)
		return 0;

	driver = malloc(sizeof(struct s725_driver));
	if (!driver)
		return 0;

	bzero(driver, sizeof(struct s725_driver));

	switch (driver_type) {
	case DRIVER_SERIAL:
		if (device != NULL) {
			driver->dops = &serial_driver_ops;
			strncpy(driver->path, device, sizeof(driver->path)-1);
			driver->uses_frames = 1;
			return 1;
		}
		break;
	}

	free(driver);
	driver = NULL;
	return 0;
}

int
driver_write(BUF *buf)
{
	if (driver->dops->write)
		return driver->dops->write(driver, buf);
	else
		return -1;
}

int
driver_read(BUF *buf)
{
	if (driver->dops->read)
		return driver->dops->read(driver, buf);
	else
		return 0;
}

int
driver_read_byte(unsigned char *b)
{
	if (driver->dops->read_byte)
		return driver->dops->read_byte(driver, b);
	else
		return 0;
}

int
driver_open()
{
	int ret = -1;

	if (driver->dops->init)
		ret = driver->dops->init(driver);

	return ret;
}

int
driver_close()
{
	int ret = 0;

	if (driver->dops->close)
		ret = driver->dops->close(driver);

	free(driver);
	driver = NULL;
	return ret;
}

int
driver_uses_frames()
{
	return driver->uses_frames;
}

int
driver_name_to_type(const char *driver_name)
{
	if (!strcmp(driver_name, "serial")) {
		return DRIVER_SERIAL;
	} else {
		return DRIVER_UNKNOWN;
	}
}

const char*
driver_type_to_name(int driver_type)
{
	switch (driver_type) {
	case DRIVER_SERIAL:
		return "serial";
		break;
	default:
		return "unknown";
	}
}

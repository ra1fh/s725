
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver.h"
#include "driver_int.h"

extern struct s725_driver_ops serial_driver_ops;
extern struct s725_driver_ops ir_driver_ops;
extern struct s725_driver_ops stir_driver_ops;

static struct s725_driver *driver;

int
driver_init(const int driver_type, const char *device)
{
	int needpath;

	if (driver)
		return 0;
	
	driver = malloc(sizeof(struct s725_driver));
	if (!driver)
		return 0;

	bzero(driver, sizeof(struct s725_driver));

	switch (driver_type) {
	case DRIVER_SERIAL:
		driver->dops = &serial_driver_ops;
		needpath = 1;
		break;
	case DRIVER_IR:
		driver->dops = &ir_driver_ops;
		needpath = 1;
		break;
	case DRIVER_STIR:
		driver->dops = &stir_driver_ops;
		needpath = 0;
		break;
	default:
		fprintf(stderr, "unknown driver type: %d\n", driver_type);
		return 0;
	}

	if (!needpath)
		return 1;

	if (device == NULL)
		return 0;
	
	strncpy(driver->path,device,sizeof(driver->path)-1);
	return 1;
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
driver_read_byte(unsigned char *b)
{
	if (driver->dops->read)
		return driver->dops->read(driver, b);
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

	return ret;
}

int
driver_name_to_type(const char *driver_name)
{
	if (!strcmp(driver_name, "ir")) {
		return DRIVER_IR;
	} else if (!strcmp(driver_name, "serial")) {
		return DRIVER_SERIAL;
	} else if (!strcmp(driver_name, "stir")) {
		return DRIVER_STIR;
	} else {
		return DRIVER_UNKNOWN;
	}
}
	   
const char*
driver_type_to_name(int driver_type)
{
	switch (driver_type) {
	case DRIVER_IR:
		return "ir";
		break;
	case DRIVER_SERIAL:
		return "serial";
		break;
	case DRIVER_STIR:
		return "stir";
		break;
	default:
		return "unknown";
	}
}

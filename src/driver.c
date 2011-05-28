
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver.h"
#include "driver_int.h"

extern struct s710_driver_ops serial_driver_ops;
extern struct s710_driver_ops ir_driver_ops;
extern struct s710_driver_ops usb_driver_ops;

static struct s710_driver *driver;

int
driver_init (const char *driver_name, const char *device)
{
	int  init = 0;
	int  needpath = 1;

	if (driver)
		return 0;
	
	driver = malloc(sizeof(struct s710_driver));
	if (!driver)
		return 0;

	bzero(driver, sizeof(struct s710_driver));

	if (driver_name) {
		if ( !strcmp(driver_name,"serial") ) {
			driver->dops = &serial_driver_ops;
		} else if ( !strcmp(driver_name,"ir") ) {
			driver->dops = &ir_driver_ops;
		} else if ( !strcmp(driver_name,"usb") ) {
			driver->dops = &usb_driver_ops;
			needpath = 0;
			printf("needpath=0\n");
		}
	}

	if ( needpath != 0 && device != NULL ) {
		strncpy(driver->path,device,sizeof(driver->path)-1);
		init = 1;
	} else if ( needpath == 0 && device == 0 ) {
		init = 1;
	}

	return init;
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

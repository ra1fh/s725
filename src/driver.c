
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "s710.h"

static struct s710_driver_ops serial_driver_ops = {
	.init = serial_init,
	.read = serial_read_byte,
	.write = serial_write,
	.close = NULL,
};

static struct s710_driver_ops ir_driver_ops = {
	.init = ir_init,
	.read = ir_read_byte,
	.write = ir_write,
	.close = NULL,
};

static struct s710_driver_ops usb_driver_ops = {
	.init = usb_init_port,
	.read = usb_read_byte,
	.write = usb_send_packet,
	.close = usb_shutdown_port,
};

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
	driver->type = S710_DRIVER_SERIAL;

	if (driver_name) {
		if ( !strcmp(driver_name,"serial") ) {
			driver->type = S710_DRIVER_SERIAL;
			driver->dops = &serial_driver_ops;
		} else if ( !strcmp(driver_name,"ir") ) {
			driver->type = S710_DRIVER_IR;
			driver->dops = &ir_driver_ops;
		} else if ( !strcmp(driver_name,"usb") ) {
			driver->type = S710_DRIVER_USB;
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
driver_write(unsigned char *buf, size_t nbytes) {
	if (driver->dops->write)
		return driver->dops->write(driver, buf, nbytes);
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
driver_open(S710_Mode mode)
{
	int ret = -1;

	driver->mode = mode;
	if (driver->dops->init)
		ret = driver->dops->init(driver, mode);

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

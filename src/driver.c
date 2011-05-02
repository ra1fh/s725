#include <unistd.h>
#include <string.h>
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
	.init = init_usb_port,
	.read = read_usb_byte,
	.write = send_packet_usb,
	.close = NULL,
};

int
driver_init (const char *driver_name, const char *device, struct s710_driver *d)
{
	int  init = 0;
	int  needpath = 1;

	d->type = S710_DRIVER_SERIAL;

	if (driver_name) {
		if ( !strcmp(driver_name,"serial") ) {
			d->type = S710_DRIVER_SERIAL;
			d->dops = &serial_driver_ops;
		} else if ( !strcmp(driver_name,"ir") ) {
			d->type = S710_DRIVER_IR;
			d->dops = &ir_driver_ops;
		} else if ( !strcmp(driver_name,"usb") ) {
			d->type = S710_DRIVER_USB;
			d->dops = &usb_driver_ops;
			needpath = 0;
			printf("needpath=0\n");
		}
	}

	if ( needpath != 0 && device != NULL ) {
		strncpy(d->path,device,sizeof(d->path)-1);
		init = 1;
	} else if ( needpath == 0 && device == 0 ) {
		init = 1;
	}

	return init;
}

int
driver_write(struct s710_driver *d, unsigned char *buf, size_t nbytes) {
	if (d->dops->write)
		return d->dops->write(d, buf, nbytes);
	else
		return -1;
}

int
driver_read_byte(struct s710_driver *d, unsigned char *b)
{
	if (d->dops->read)
		return d->dops->read(d, b);
	else
		return 0;
}

int
driver_open(struct s710_driver *d, S710_Mode mode)
{
	int ret = -1;

	d->mode = mode;
	if (d->dops->init)
		ret = d->dops->init(d, mode);

	return ret;
}

int
driver_close (struct s710_driver *d)
{
	int ret = 0;

	if (d->dops->close)
		ret = d->dops->close(d);

	return ret;
}

#include <unistd.h>
#include <string.h>
#include "s710.h"


/* externs */

extern char *optarg;
extern int   optind;


int
driver_init (const char *driver_name, const char *device, S710_Driver *d)
{
	int  init = 0;
	int  needpath = 1;

	d->type = S710_DRIVER_SERIAL;

	if (driver_name) {
		if ( !strcmp(driver_name,"serial") ) {
			d->type = S710_DRIVER_SERIAL;
		} else if ( !strcmp(driver_name,"ir") ) {
			d->type = S710_DRIVER_IR;
		} else if ( !strcmp(driver_name,"usb") ) {
			d->type = S710_DRIVER_USB;
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
driver_open ( S710_Driver *d, S710_Mode mode )
{
	int ret = -1;

	d->mode = mode;

	switch ( d->type ) {
	case S710_DRIVER_SERIAL:
		compute_byte_map();
	case S710_DRIVER_IR:
		ret = init_serial_port(d,mode);
		break;
	case S710_DRIVER_USB:
		ret = init_usb_port(d);
		break;
	default:
		break;
	}

	return ret;
}


int
driver_close ( S710_Driver *d )
{
	int ret = 0;

	switch ( d->type ) {
	case S710_DRIVER_SERIAL:    
	case S710_DRIVER_IR:
		ret = close((int)d->data);
		break;
	case S710_DRIVER_USB:
		ret = shutdown_usb_port(d);
		break;
	default:
		break;
	}

	return ret;
}

/*
 * irda driver
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


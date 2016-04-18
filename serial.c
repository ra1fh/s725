/*
 * serial/ir driver
 */

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "driver_int.h"
#include "xmalloc.h"

#define SERIAL_READ_TRIES 10

static int ir_init(struct s725_driver *d);
static int ir_read_byte(struct s725_driver *d, unsigned char *byte);
static int ir_write(struct s725_driver *d, BUF *buf);

static int serial_init(struct s725_driver *d);
static int serial_write(struct s725_driver *d, BUF *buf);
static int serial_read_byte(struct s725_driver *d, unsigned char *byte);

static void compute_byte_map(void);

struct s725_driver_ops serial_driver_ops = {
	.init = serial_init,
	.read = serial_read_byte,
	.write = serial_write,
	.close = NULL,
};

struct s725_driver_ops ir_driver_ops = {
	.init = ir_init,
	.read = ir_read_byte,
	.write = ir_write,
	.close = NULL,
};

struct driver_private {
	int fd;
};

#define DP(x) ((struct driver_private *)x->data)

unsigned char gByteMap[256];

/* 
 * initialize the serial port
 */
static int  
serial_init(struct s725_driver *d)
{
	struct termios t;
	int fd;
	compute_byte_map();

	fd = open(d->path, O_RDWR | O_NOCTTY | O_NDELAY); 
	if (fd < 0) { 
		fprintf(stderr,"%s: %s\n",d->path,strerror(errno)); 
		return -1; 
	}

	fcntl(fd,F_SETFL,O_RDONLY);
	memset(&t,0,sizeof(t));

	/* 9600 bps, 8 data bits, 1 or 2 stop bits (??), no parity */

	t.c_cflag = B9600 | CS8 | CLOCAL | CREAD;

	/* I don't know why an extra stop bit makes it work for bidirectional
	   communication.  Also, it doesn't work for everyone - in fact, it
	   may only work for me. */

	t.c_cflag |= CSTOPB;
	t.c_iflag = IGNPAR;
	t.c_oflag = 0;
	t.c_lflag = 0;

	t.c_cc[VTIME]    = 1; /* inter-character timer of 0.1 second used */
	t.c_cc[VMIN]     = 0;  /* blocking read until 1 char / timer expires */

	/* set up for input */

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&t);

	d->data = xmalloc(sizeof(struct driver_private));
	DP(d)->fd = fd;

	return fd;
}

static void
serial_debug(unsigned char *byte, int r)
{
#if DEBUG
	static struct timeval ti;
	static struct timeval tf;
	static int n;
	float el;

	gettimeofday(&tf,NULL);
	el = (n++) ? (float)tf.tv_sec-ti.tv_sec+(tf.tv_usec-ti.tv_usec)/1000000.0 : 0;
	memcpy(&ti,&tf,sizeof(struct timeval));

	fprintf(stderr,"%4d: %f",n,el);
	if (r != 0) {
		fprintf(stderr," [%02x]\n",*byte);
	} else {
		fprintf(stderr,"\n");
	}
#endif
}
	
static int
serial_read_byte(struct s725_driver *d, unsigned char *byte)
{
	int r = 0;
	int i = SERIAL_READ_TRIES;

	do {
		r = read(DP(d)->fd,byte,1);
	} while (!r && i--);

	serial_debug(byte, r);

	return r;
}

static int
serial_write(struct s725_driver *d, BUF *buf)
{
	unsigned int i;
	unsigned char c;
	int ret = 0;

	for (i = 0; i < buf_len(buf); i++) {
		c = buf_getc(buf, i);
		if (write(DP(d)->fd, &gByteMap[c], 1) < 0)
			ret = -1;
	}
    
	/*
	 * the data that gets echoed back is not RS-232 data.  it is garbage 
	 * data that we have to flush.  there is a pause of at least 0.1 
	 * seconds before the real data shows up.
	 */
	usleep(100000);
	tcflush(DP(d)->fd,TCIFLUSH);
	return ret;
}

static int
ir_init(struct s725_driver *d)
{
	return serial_init(d);
}

static int
ir_read_byte(struct s725_driver *d, unsigned char *byte)
{
	return serial_read_byte(d, byte);
}

static int
ir_write(struct s725_driver *d, BUF *buf)
{
	unsigned int i;
	unsigned char c;
	int ret = 0;
	int write_single_chunk = 1;

	if (write_single_chunk) {
		if ((write(DP(d)->fd, buf_get(buf), buf_len(buf)) < 0))
			ret = -1;
	} else {
		for (i = 0; i < buf_len(buf); i++)  {
			c = buf_getc(buf, i);
			if ((write(DP(d)->fd, &c, 1)) < 0)
				ret = -1;
		}
	}
    
	/*
	 * the data that gets echoed back is not RS-232 data.  it is garbage 
	 * data that we have to flush.  there is a pause of at least 0.1 
	 * seconds before the real data shows up.
	 */
	usleep(100000);
	tcflush(DP(d)->fd,TCIFLUSH);
	return ret;
}

static void
compute_byte_map(void)
{
	int i, j, m;

	for (i = 0; i < 0x100; i++) {
		m = !(i & 1);
		for (j=7;j>0;j--)
			m |= ((i+0x100-(1<<(j-1)))&0xff) & (1<<j);
		gByteMap[m] = i;
	}
}


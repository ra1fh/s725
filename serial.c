/*
 * serial/ir driver
 */

#include <sys/ioctl.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "driver_int.h"
#include "xmalloc.h"

#define SERIAL_READ_TRIES 10

static int serial_init(struct s725_driver *d);
static int serial_write(struct s725_driver *d, BUF *buf);
static int serial_read_byte(struct s725_driver *d, unsigned char *byte);
static int serial_close(struct s725_driver *d);

struct s725_driver_ops serial_driver_ops = {
	.init = serial_init,
	.read = serial_read_byte,
	.write = serial_write,
	.close = serial_close,
};

struct driver_private {
	int fd;
	int debugfd;
	struct termios tio;
};

#define DP(x) ((struct driver_private *)x->data)

unsigned char gByteMap[256];

static void
serial_print_bits(unsigned int out)
{
	unsigned int mask = 0x8000;
	for (; mask; mask >>=1) {
		if ((mask & 0x08888888))
			fprintf(stderr, " ");
		fprintf(stderr, "%hhu", out & mask ? 1:0);
	}
}

static void
serial_print_termios(struct termios *tio, char *label)
{
	int i;
	int cc = 0;

	if (label) {
		fprintf(stderr, "termios state: %s\n", label);
	}
	
	fprintf(stderr, "c_iflag: 0x%08x ", tio->c_iflag);
	serial_print_bits(tio->c_iflag);
	fprintf(stderr, "\nc_oflag: 0x%08x ", tio->c_oflag);
	serial_print_bits(tio->c_oflag);
	fprintf(stderr, "\nc_cflag: 0x%08x ", tio->c_cflag);
	serial_print_bits(tio->c_cflag);
	fprintf(stderr, "\nc_lflag: 0x%08x ", tio->c_lflag);
	serial_print_bits(tio->c_lflag);
	fprintf(stderr, "\nc_ispeed: %d\n", tio->c_ispeed);
	fprintf(stderr, "c_ospeed: %d\n", tio->c_ospeed);

	if (cc) {
		for (i = 0; i < NCCS; ++i) {
			fprintf(stderr, "c_cc[%02d] = %3hhu ", i, tio->c_cc[i]);
			switch(i) {
			case 16:
				fprintf(stderr, "(VTIME)\n");
				break;
			case 17:
				fprintf(stderr, "(VMIN)\n");
				break;
			default:
				fprintf(stderr, "\n");
			}
		}
		fprintf(stderr, "\n");
	}
}

/* 
 * initialize the serial port
 */
static int  
serial_init(struct s725_driver *d)
{
	struct termios t;
	int fd;

	fd = open(d->path, O_RDWR | O_NOCTTY | O_NDELAY); 
	if (fd < 0) { 
		fprintf(stderr,"%s: %s\n",d->path,strerror(errno)); 
		return -1; 
	}

	if (tcgetattr(fd, &t) == -1) {
		fprintf(stderr,"%s: %s\n", d->path, strerror(errno)); 
		return -1; 
	}

	serial_print_termios(&t, "inital state");
	
	d->data = xmalloc(sizeof(struct driver_private));
	DP(d)->fd = fd;
	DP(d)->tio = t;

	cfmakeraw(&t);

	serial_print_termios(&t, "after cmakeraw");

	t.c_cflag |= CLOCAL;

	/* 8 bits */
	t.c_cflag &= ~CSIZE;
	t.c_cflag |= CS8;

	/* no parity */
	t.c_iflag &= ~(PARMRK | INPCK);
	t.c_iflag |= IGNPAR;
	t.c_cflag &= ~PARENB;

	/* 2 stop bits */
	// t.c_cflag |= CSTOPB;
	t.c_cflag &= ~CSTOPB;

	/* no flow control */
	t.c_cflag &= ~CRTSCTS;
	t.c_iflag &= ~(IXON | IXOFF | IXANY);;

	if (cfsetispeed(&t, B9600) < 0) {
		fprintf(stderr,"%s: %s\n", d->path, strerror(errno)); 
		return -1;
	}
		
	if ((cfsetospeed(&t, B9600)) < 0) {
		fprintf(stderr,"%s: %s\n", d->path, strerror(errno)); 
		return -1;
	}
		
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	
	serial_print_termios(&t, "modified state");
	
	if (tcsetattr(fd, TCSANOW, &t) == -1) {
		fprintf(stderr,"%s: %s\n", d->path, strerror(errno)); 
		return -1; 
	}

	/*
	 * IRXON IR220 based serial-IR converters use DTR/RTS as power
	 * supply. It takes some time for the device to initialize.
	 * DTR and RTS are pulled to +12V when the device node is opened,
	 * so wait some millisecond, 50ms seems a good value.
	 */
 	usleep(50000);
	
	return fd;
}

static int
serial_close(struct s725_driver *d)
{
	fprintf(stderr, "serial_close\n");

	serial_print_termios(&DP(d)->tio, "restored state");
	close(DP(d)->fd);
	close(DP(d)->debugfd);
	return 0;
}
	
static int
serial_read_byte(struct s725_driver *d, unsigned char *byte)
{
	struct pollfd pfd[1];
	int nready;

	pfd[0].fd = DP(d)->fd;
	pfd[0].events = POLLIN;

	int r = 0;
	int ntries = SERIAL_READ_TRIES;

	do {
		nready = poll(pfd, 1, 100);
		if (nready == -1) {
			r = 0;
			fprintf(stderr, "poll: %s\n", strerror(errno));
		}
		if (nready == 0) {
			r = 0;
			fprintf(stderr, "poll: timeout\n");
		}
		if ((pfd[0].revents & (POLLERR|POLLNVAL))) {
			r = 0;
			fprintf(stderr, "poll: bad fd %d\n", pfd[0].fd);
		}
		if ((pfd[0].revents & (POLLIN|POLLHUP))) {
			r = read(DP(d)->fd,byte,1);
			if (r == -1) {
				fprintf(stderr, "read: error %s\n", strerror(errno));
				r = 0;
			} else {
				fprintf(stderr, "read: 0x%02hhx\n", *byte);
			}
		   
		}
	} while (!r && ntries--);

	return r;
}

static int
serial_write(struct s725_driver *d, BUF *buf)
{
	unsigned int i;
	unsigned char c;
	int ret = 0;
	int write_single_chunk = 1;

	tcflush(DP(d)->fd, TCIFLUSH);

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

	tcdrain(DP(d)->fd);
	return ret;
}


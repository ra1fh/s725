/* serial.c - serial/ir driver */

/*
 * Copyright (C) 2016 Ralf Horstmann
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
#include "log.h"
#include "xmalloc.h"

#define SERIAL_READ_TRIES 10

static int serial_init(struct s725_driver *d);
static int serial_write(struct s725_driver *d, BUF *buf);
static int serial_read_byte(struct s725_driver *d, unsigned char *byte);
static int serial_close(struct s725_driver *d);

struct s725_driver_ops serial_driver_ops = {
	.init = serial_init,
	.read = NULL,
	.read_byte = serial_read_byte,
	.write = serial_write,
	.close = serial_close,
};

struct driver_private {
	int fd;
	struct termios tio;
};

#define DP(x) ((struct driver_private *)x->data)

static void
serial_print_bits(unsigned int out)
{
	unsigned int mask = 0x8000;

	if (log_get_level() < 1)
		return;

	for (; mask; mask >>=1) {
		if ((mask & 0x08888888))
			log_write(" ");
		log_write("%u", out & mask ? 1 : 0);
	}
}

static void
serial_print_termios(struct termios *tio, char *label)
{
	int i;
	int cc = 0;

	if (log_get_level() < 1)
		return;

	if (label) {
		log_write("termios state: %s\n", label);
	}

	log_write("c_iflag: 0x%08x ", tio->c_iflag);
	serial_print_bits(tio->c_iflag);
	log_write("\nc_oflag: 0x%08x ", tio->c_oflag);
	serial_print_bits(tio->c_oflag);
	log_write("\nc_cflag: 0x%08x ", tio->c_cflag);
	serial_print_bits(tio->c_cflag);
	log_write("\nc_lflag: 0x%08x ", tio->c_lflag);
	serial_print_bits(tio->c_lflag);
	log_write("\nc_ispeed: %d\n", tio->c_ispeed);
	log_write("c_ospeed: %d\n", tio->c_ospeed);

	if (cc) {
		for (i = 0; i < NCCS; ++i) {
			log_write("c_cc[%02d] = %3hhu ", i, tio->c_cc[i]);
			switch(i) {
			case 16:
				log_write("(VTIME)\n");
				break;
			case 17:
				log_write("(VMIN)\n");
				break;
			default:
				log_write("\n");
			}
		}
		log_write("\n");
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
		log_error("%s: %s", d->path, strerror(errno));
		return -1;
	}

	if (tcgetattr(fd, &t) == -1) {
		log_error("%s: %s", d->path, strerror(errno));
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
	t.c_cflag &= ~CSTOPB;

	/* no flow control */
	t.c_cflag &= ~CRTSCTS;
	t.c_iflag &= ~(IXON | IXOFF | IXANY);;

	if (cfsetispeed(&t, B9600) < 0) {
		log_error("%s: %s", d->path, strerror(errno));
		return -1;
	}

	if ((cfsetospeed(&t, B9600)) < 0) {
		log_error("%s: %s", d->path, strerror(errno));
		return -1;
	}

	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;

	serial_print_termios(&t, "modified state");

	if (tcsetattr(fd, TCSANOW, &t) == -1) {
		log_error("%s: %s", d->path, strerror(errno));
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
	log_info("serial_close");
	/*
	 * Need to wait here as well to give serial IR converters enough
	 * time to transmit, even though we do tcdrain after write.
	 */
 	usleep(50000);
	close(DP(d)->fd);
	xfree(DP(d));
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
			log_info("serial_read_byte: poll returned %s", strerror(errno));
		}
		if (nready == 0) {
			r = 0;
			log_debug("serial_read_byte: poll timeout");
		}
		if ((pfd[0].revents & (POLLERR|POLLNVAL))) {
			r = 0;
			log_error("serial_read_byte: poll bad fd %d", pfd[0].fd);
		}
		if ((pfd[0].revents & (POLLIN|POLLHUP))) {
			r = read(DP(d)->fd,byte,1);
			if (r == -1) {
				log_error("serial_read_byte: error %s", strerror(errno));
				r = 0;
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

	log_info("serial_write: len=%zu", buf_len(buf));
	if (log_get_level() >= 2)
		log_hexdump(buf_get(buf), buf_len(buf));

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

/* irda.c - irda driver using Linux irda socket */

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

#if defined(__linux__)
#include <sys/socket.h>
#include <linux/irda.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#endif

#include "driver_int.h"
#include "log.h"
#include "xmalloc.h"

#define IRDA_READ_TRIES 10

static int irda_init(struct s725_driver *d);
static int irda_write(struct s725_driver *d, BUF *buf);
static int irda_read(struct s725_driver *d, BUF *buf);
static int irda_read_byte(struct s725_driver *d, unsigned char *byte);
static int irda_close(struct s725_driver *d);

struct s725_driver_ops irda_driver_ops = {
	.init = irda_init,
	.read = irda_read,
	.read_byte = irda_read_byte,
	.write = irda_write,
	.close = irda_close,
};

#if defined(__linux__)

struct driver_private {
	int fd;
};

#define DP(x) ((struct driver_private *)x->data)
#define IRDA_DEVICES

static unsigned int
irda_discover(int fd, uint32_t *daddr)
{
    struct irda_device_list *list;
    unsigned char buf[sizeof(struct irda_device_info) * (IRDA_DEVICES + 1)];
    unsigned int len;

    len = sizeof(struct irda_device_list) * (IRDA_DEVICES + 1);

	log_info("len=%d, sizeof(buf)=%d", len, sizeof(buf));
	
    list = (struct irda_device_list *) buf;
        
	log_info("irda_discover: starting discovery on fd=%d", fd);
	
	
	if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &len) != 0) {
		log_error("irda_discover: getsockopt failed (%s)", strerror(errno));
		return 0;
	}

	if (len <= 0) {
		log_error("irda_discover: no devices found");
		return 0;
	}
		
	log_info("irda_discover: len=%d", list->len);
	log_info("irda_discover: name=%s", list->dev[0].info);
	log_info("irda_discover: daddr=%08x", list->dev[0].daddr);
	log_info("irda_discover: saddr=%08x", list->dev[0].saddr);

	*daddr = list->dev[0].daddr;
	
    return 1;
}

/* 
 * initialize the irda port
 */
static int  
irda_init(struct s725_driver *d)
{
	struct sockaddr_irda peer;
	uint32_t daddr;
	int fd;

	memset(&peer, 0, sizeof(peer));
	
	fd = socket(AF_IRDA, SOCK_STREAM, 0);
	if (! irda_discover(fd, &daddr)) {
		close(fd);
		return -1;
	}

	log_info("irda_init: connecting to %x08x", daddr);
	
	peer.sir_family = AF_IRDA;
	peer.sir_lsap_sel = LSAP_ANY;
	peer.sir_addr = daddr;
	strcpy(peer.sir_name, "HRM");
	
	if (connect(fd, (struct sockaddr*) &peer, sizeof(struct sockaddr_irda)) == -1) {
		log_error("irda_init: connect failed");
		close(fd);
		return -1;
	}

	d->data = xmalloc(sizeof(struct driver_private));
	DP(d)->fd = fd;

	log_info("irda_init: connected");
	return DP(d)->fd;
}

static int
irda_close(struct s725_driver *d)
{
	log_info("irda_close");
	close(DP(d)->fd);
	xfree(DP(d));

	return 0;
}

static int
irda_read(struct s725_driver *d, BUF *buf)
{
	int r;

	log_info("irda_read: buf=%08x, buf len=%d", (long long) buf, buf_len(buf));
	r = recv(DP(d)->fd, buf_get(buf), buf_capacity(buf), 0);
	if (r >= 0) {
		buf_set_len(buf, r);
	}
	return r;
}

static int
irda_read_byte(struct s725_driver *d, unsigned char *byte)
{
	struct pollfd pfd[1];
	int nready;

	pfd[0].fd = DP(d)->fd;
	pfd[0].events = POLLIN;

	int r = 0;
	int ntries = IRDA_READ_TRIES;

	do {
		nready = poll(pfd, 1, 100);
		if (nready == -1) {
			r = 0;
			log_info("irda_read_byte: poll returned %s", strerror(errno));
		}
		if (nready == 0) {
			r = 0;
			log_debug("irda_read_byte: poll timeout");
		}
		if ((pfd[0].revents & (POLLERR|POLLNVAL))) {
			r = 0;
			log_error("irda_read_byte: poll bad fd %d", pfd[0].fd);
		}
		if ((pfd[0].revents & (POLLIN|POLLHUP))) {
			r = read(DP(d)->fd,byte,1);
			if (r == -1) {
				log_error("irda_read_byte: error %s", strerror(errno));
				r = 0;
			}
		}
	} while (!r && ntries--);

	return r;
}

static int
irda_write(struct s725_driver *d, BUF *buf)
{
	int ret = 0;
	
	log_info("irda_write: len=%zu", buf_len(buf));

	if (log_get_level() >= 2)
		log_hexdump(buf_get(buf), buf_len(buf));
	
	if ((write(DP(d)->fd, buf_get(buf), buf_len(buf)) < 0))
		ret = -1;

	return ret;
}

#else

static int
irda_init(struct s725_driver *d)
{
	fatalx("irda driver not supported");
	return -1;
}

static int
irda_close(struct s725_driver *d)
{
	fatalx("irda driver not supported");
	return -1;
}
	
static int
irda_read(struct s725_driver *d, BUF *buf)
{
	fatalx("irda driver not supported");
	return -1;
}

static int
irda_read_byte(struct s725_driver *d, unsigned char *byte)
{
	fatalx("irda driver not supported");
	return -1;
}

static int
irda_write(struct s725_driver *d, BUF *buf)
{
	fatalx("irda driver not supported");
	return -1;
}

#endif

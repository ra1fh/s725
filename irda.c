/* irda driver */

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
#include <string.h>
#include <unistd.h>
#endif

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

#if defined(__linux__)

struct driver_private {
	int fd;
};

#define DP(x) ((struct driver_private *)x->data)
#define IRDA_DEVICES

static int
irda_discover(int fd)
{
    struct irda_device_list *list;
    unsigned char buf[sizeof(struct irda_device_info) * (IRDA_DEVICES + 1)];
    unsigned int len;

    len = sizeof(struct irda_device_list) * (IRDA_DEVICES + 1);

	log_info("len=%d, sizeof(buf)=%d", len, sizeof(buf));
	
    list = (struct irda_device_list *) buf;
        
	log_info("irda_discover: starting discovery");

	if (! getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &len)) {
		log_error("irda_discover: getsockopt failed (%s)", strerror(errno));
		return -1;
	}

	if (len <= 0) {
		log_error("irda_discover: no devices found");
		return -1;
	}
		
	log_info("irda_discover: len=%d", list->len);
	log_info("irda_discover: name=%s", list->dev[0].info);
	log_info("irda_discover: daddr=%08x", list->dev[0].daddr);
	log_info("irda_discover: saddr=%08x", list->dev[0].saddr);

    return list->dev[0].daddr;
}

/* 
 * initialize the irda port
 */
static int  
irda_init(struct s725_driver *d)
{
	struct sockaddr_irda peer;
	int daddr;
	int fd;
	
	fd = socket(AF_IRDA, SOCK_STREAM, 0);
	daddr = irda_discover(fd);
	if (daddr < 0) {
		close(fd);
		return -1;
	}

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

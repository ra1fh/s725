/*
 * stir driver for SigmaTel STIr4200 USB IrDA bridges
 *
 * Copyright (C) 2016 Ralf Horstmann
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/time.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb.h>

#include "driver_int.h"
#include "log.h"

static int stir_init_port(struct s725_driver *d);
static int stir_read_byte(struct s725_driver *d, unsigned char *byte);
static int stir_send_packet(struct s725_driver *d, BUF *buf);
static int stir_shutdown_port(struct s725_driver *d);
static int stir_write_reg(libusb_device_handle *handle, uint16_t reg, uint8_t value);
	
struct s725_driver_ops stir_driver_ops = {
	.init = stir_init_port,
	.read = NULL,
	.read_byte = stir_read_byte,
	.write = stir_send_packet,
	.close = stir_shutdown_port,
};

struct s725_stir_data {
	libusb_device_handle *handle;
	int endpoint_in;
	int endpoint_out;
	int interface;
};

enum StirRequests {
	REQ_WRITE_REG =		0x00,
	REQ_READ_REG =		0x01,
	REQ_READ_ROM =		0x02,
	REQ_WRITE_SINGLE =	0x03,
};

enum StirRegs {
	REG_RSVD=0,
	REG_MODE,
	REG_PDCLK,
	REG_CTRL1,
	REG_CTRL2,
	REG_FIFOCTL,
	REG_FIFOLSB,
	REG_FIFOMSB,
	REG_DPLL,
	REG_IRDIG,
	REG_TEST=15,
};

enum StirModeMask {
	MODE_FIR = 0x80,
	MODE_SIR = 0x20,
	MODE_ASK = 0x10,
	MODE_FASTRX = 0x08,
	MODE_FFRSTEN = 0x04,
	MODE_NRESET = 0x02,
	MODE_2400 = 0x01,
};

enum StirPdclkMask {
	PDCLK_4000000 = 0x02,
	PDCLK_115200 = 0x09,
	PDCLK_57600 = 0x13,
	PDCLK_38400 = 0x1D,
	PDCLK_19200 = 0x3B,
	PDCLK_9600 = 0x77,
	PDCLK_2400 = 0xDF,
};

enum StirCtrl1Mask {
	CTRL1_SDMODE = 0x80,
	CTRL1_RXSLOW = 0x40,
	CTRL1_TXPWD = 0x10,
	CTRL1_RXPWD = 0x08,
	CTRL1_SRESET = 0x01,
};

enum StirCtrl2Mask {
	CTRL2_SPWIDTH = 0x08,
	CTRL2_REVID = 0x03,
};

enum StirFifoCtlMask {
	FIFOCTL_DIR = 0x10,
	FIFOCTL_CLR = 0x08,
	FIFOCTL_EMPTY = 0x04,
};

#define S725_STIR_VENDOR_ID  0x066f
#define S725_STIR_PRODUCT_ID 0x4200
#define S725_STIR_INTERFACE 0
#define FIFO_REGS_SIZE 3
#define STIR_FIFO_SIZE 4096


int
stir_open_device(struct s725_stir_data *data)
{
	libusb_device **list;
	libusb_device *found = NULL;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	const struct libusb_interface_descriptor *intf;
	ssize_t i = 0;
	int err = 0;

	data->handle = NULL;
	data->endpoint_in = 0;
	data->endpoint_out = 0;
	data->interface = 0;
	
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	if (cnt < 0) {
		log_error("stir_open_device: error: %s", libusb_strerror(cnt));
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		libusb_get_device_descriptor(device, &desc);
		if (desc.idVendor == S725_STIR_VENDOR_ID &&
			desc.idProduct == S725_STIR_PRODUCT_ID) {
			found = device;
			break;
		}
	}

	if (!found) {
		log_error("stir_open_device: error: device not found");
		err = -1;
		goto out;
	}

	err = libusb_open(found, &data->handle);
	if (err) {
		log_error("stir_open_device: error: %s", libusb_strerror(err));
		err = -1;
		goto out;
	}

	err = libusb_set_configuration(data->handle, 1);
	if (err) {
		log_error("stir_open_device: error: %s", libusb_strerror(err));
		err = -1;
		goto out;
	}
	
	err = libusb_get_active_config_descriptor(found, &config);
	if (err) {
		log_error("stir_open_device: error: %s", libusb_strerror(err));
		err = -1;
		goto out;
	}

	if (config->bNumInterfaces != 1) {
		log_error("stir_open_device: error: bNumInterfaces != 1");
		err = -1;
		goto out;
	}

	if (config->interface[0].num_altsetting != 1) {
		log_error("stir_open_device: error: num_altsetting != 1 (%d)", config->interface[0].num_altsetting);
		err = -1;
		goto out;
	}

	intf = &config->interface[0].altsetting[0];
	data->interface = intf->bInterfaceNumber;
	log_info("stir_open_device: interface: %d", data->interface);

	for (i = 0; i < intf->bNumEndpoints; i++) {
		if ((intf->endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN &&
			(intf->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)
			data->endpoint_in = intf->endpoint[i].bEndpointAddress;

		if ((intf->endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT &&
			(intf->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)
			data->endpoint_out = intf->endpoint[i].bEndpointAddress;
	}

	log_info("stir_open_device: ep_in: %x", data->endpoint_in);
	log_info("stir_open_device: ep_out: %x", data->endpoint_out);

	if (data->endpoint_in == 0 || data->endpoint_out == 0) {
		log_error("stir_open_device: error: endpoints not found");
		err = -1;
		goto out;
	}
	
	err = libusb_claim_interface(data->handle, data->interface);
	if (err) {
		log_error("stir_open_device: error: %s", libusb_strerror(err));
		err = -1;
		goto out;
	}

	log_info("stir_open_device: claimed interface");

	err = 0;
out:	
	libusb_free_device_list(list, 1);
	return err;
}

static int
stir_init_port(struct s725_driver *d)
{
	struct s725_stir_data *data;
	int err;

	if ((err = libusb_init(NULL)) != 0) {
		log_error("stir_init_port: error: %s", libusb_strerror(err));
		return -1;
	}
	
	libusb_set_debug(NULL, 1);

	if ((data = calloc(1,sizeof(struct s725_stir_data))) == NULL) {
		log_error("stir_init_port: allocation failed");
		return -1;
	}
	
	if ((err = stir_open_device(data)) != 0) {
		free(data);
		return -1;
	}

	log_info("stir_init_port: *** clear halt out");
	err = libusb_clear_halt(data->handle, 1/* data->endpoint_out */ );
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	log_info("stir_init_port: *** clear halt in");
	err = libusb_clear_halt(data->handle, 130/* data->endpoint_in */);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	log_info("stir_init_port: *** CTRL1");
	err = stir_write_reg(data->handle, REG_CTRL1, CTRL1_SRESET);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	log_info("stir_init_port: *** DPLL");
	err = stir_write_reg(data->handle, REG_DPLL, 0x15);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	log_info("stir_init_port: *** PDCLK");
	err = stir_write_reg(data->handle, REG_PDCLK, PDCLK_9600);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	log_info("stir_init_port: *** MODE");
	err = stir_write_reg(data->handle, REG_MODE, MODE_NRESET | MODE_FASTRX | MODE_SIR);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	/* SD/MODE=1 */
	log_info("stir_init_port: *** CTRL1");
	err = stir_write_reg(data->handle, REG_CTRL1, CTRL1_SDMODE);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	/* SD/MODE=0 */
	log_info("stir_init_port: *** CTRL1");
	err = stir_write_reg(data->handle, REG_CTRL1, 0x00);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	/* SPWIDTH=SIR */
	log_info("stir_init_port: *** CTRL2");
	err = stir_write_reg(data->handle, REG_CTRL2, 0x20);
	if (err) {
		log_error("stir_init_port: %s", libusb_strerror(err));
		return -1;
	}

	usleep(1000);
	
	log_info("stir_init_port: success");
	d->data = (void *)data;
	return 0;
}

/* Send control message to read multiple registers */
static int stir_read_reg(libusb_device_handle *handle, uint16_t reg, uint8_t *data, uint16_t count)
{
	int r;

	log_info("stir_read_reg: reg=%x-%x", reg, reg + count - 1);

	r = libusb_control_transfer(handle,
								LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN,
								REQ_READ_REG,
								0x00, /* wValue not used */
								reg,
								data,
								count,
								1000);
	if (r < 0)
		log_error("stir_read_reg: %s (%d)", libusb_strerror(r), r);
	else if (r != count)
		log_error("stir_read_reg: incomplete read (expected %d, got %d bytes)", count, r);
	else
		log_info("stir_read_reg: reg=%x-%x ok", reg, reg + count - 1);
	return r;
}

static int stir_write_reg(libusb_device_handle *handle, uint16_t reg, uint8_t value)
{
	int r;
	unsigned char buf[BUFSIZ];

	log_info("stir_write_reg: reg=%hx, val=%hhx", reg, value);
	r = libusb_control_transfer(handle,
								LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT,
								REQ_WRITE_SINGLE, 
								value, 
								reg,
								buf,  /* not used */
								0x00, /* not used */
								1000);
	if (r < 0)
		log_error("stir_write_reg: error %s", libusb_strerror(r));
	else
		log_info("stir_write_reg: reg=%hx, val=%hhx ok", reg, value);
	return r;
}

static int
stir_read_byte(struct s725_driver *d, unsigned char *byte)
{
	int r = 0;
	static unsigned char buf[BUFSIZ];
	static int  bytes;
	static int  idx;
	int i = 0;
	int err;
	struct s725_stir_data *data = (struct s725_stir_data *)d->data;

	if (idx == bytes) {
		idx = 0;
		do {
			err = libusb_bulk_transfer(data->handle,
									   data->endpoint_in,
									   buf,
									   sizeof(buf),
									   &bytes,
									   5000);
			if (err != 0) {
				log_error("stir_read_byte: err=%d", err);
				err = libusb_reset_device(data->handle);
				if (err != 0) {
					log_error("stir_read_byte: error: reset failed");
					return 0;
				}
			}
			if (bytes > 0 && log_get_level() >= 2) {
				log_debug("stir_read_byte: len=%d", bytes);
				log_hexdump(buf, bytes);
			}
			usleep(10000);
		} while (!bytes && i++ < 100);
		if (i >= 100) {
			log_debug("stir_read_byte: ntries==100");
		}
	}
	
	if (bytes > 0) {
		*byte = (unsigned char)buf[idx++];
		r = 1;
	}
	return r;
}

static int stir_fifo_txwait(libusb_device_handle *handle,  int space)
{
	int err;
	unsigned long count, status;
	unsigned long prev_count = 0x1fff;
	uint8_t fifo_status[FIFO_REGS_SIZE];
	
	/* Read FIFO status and count */
	for (;; prev_count = count) {
		err = stir_read_reg(handle, REG_FIFOCTL, fifo_status, 
				   FIFO_REGS_SIZE);
		if (err != FIFO_REGS_SIZE) {
			log_error("stir_fifo_txwait: error: fifo regs %s (%d)", libusb_strerror(err), err);
			return (err < 0 ? err : -1);
		}

		status = fifo_status[0];
		count = (unsigned)(fifo_status[2] & 0x1f) << 8 
			| fifo_status[1];

		log_info("stir_fifo_txwait: fifo status 0x%lx count %lu", status, count);
		
		/* is fifo receiving already, or empty */
		if (!(status & FIFOCTL_DIR) ||
		    (status & FIFOCTL_EMPTY))
			return 0;

		/* only waiting for some space */
		if (space >= 0 && STIR_FIFO_SIZE - 4 > space + count)
			return 0;

		/* queue confused */
		if (prev_count < count)
			break;

		/* estimate transfer time for remaining chars */
		usleep((count * 8000000) / 9600);
	}
			
	err = stir_write_reg(handle, REG_FIFOCTL, FIFOCTL_CLR);
	if (err < 0) 
		return err;
	err = stir_write_reg(handle, REG_FIFOCTL, 0);
	if (err < 0)
		return err;

	return 0;
}



static int
stir_send_packet(struct s725_driver *d, BUF *buf)
{
	struct s725_stir_data *data = (struct s725_stir_data *)d->data;
	BUF *txbuf = buf_alloc(0);
	int err = 0;
	int bytes;

	stir_fifo_txwait(data->handle, buf_len(txbuf));
	
	buf_putc(txbuf, 0x55);
	buf_putc(txbuf, 0xAA);
	buf_putc(txbuf, buf_len(buf) & 0xff);
	buf_putc(txbuf, (buf_len(buf) >> 8) & 0xff);
	buf_append(txbuf, buf_get(buf), buf_len(buf));
	
	log_info("stir_send_packet: *** CLR set");
	err = stir_write_reg(data->handle, REG_FIFOCTL, FIFOCTL_CLR);
	if (err)
		log_error("stir_send_packet: %s", libusb_strerror(err));

	log_info("stir_send_packet: *** CLR reset");
	err = stir_write_reg(data->handle, REG_FIFOCTL, 0);
	if (err)
		log_error("stir_send_packet: %s", libusb_strerror(err));

	if (log_get_level() >= 2) {
		log_write("stir_send_packet: len=%zu\n", buf_len(txbuf));
		log_hexdump(buf_get(txbuf), buf_len(txbuf));
	}
	
	log_info("stir_send_packet: *** start bulk transfer");
	err = libusb_bulk_transfer(data->handle,
							   data->endpoint_out,
							   buf_get(txbuf),
							   buf_len(txbuf),
							   &bytes,
							   1000);
	log_info("stir_send_packet: len=%zu err=%d transferred=%d", buf_len(txbuf), err, bytes);

	stir_fifo_txwait(data->handle, buf_len(txbuf));

	usleep(1000000);

	stir_fifo_txwait(data->handle, buf_len(txbuf));
	
	buf_free(txbuf);
	return err;
}

static int
stir_shutdown_port(struct s725_driver *d)
{
	struct s725_stir_data *data = (struct s725_stir_data *)d->data;

	log_info("stir_shutdown_port: releasing interface");
	libusb_release_interface(data->handle, data->interface);

	log_info("stir_shutdown_port: closing");
	libusb_close(data->handle);

	free(data);
	d->data = (void *)NULL;

	return 0;
}




s725
====

### About

Communicate with Polar S725X heart rate monitor via IR interfaces.
This project is based on s710 from http://code.google.com/p/s710/.

### Building

Building s725 requires libusb 1.0 or later, GNU make, flex,
yacc/byacc/bison:

	# Debian/Ubuntu
	apt-get install gcc make flex bison pkg-config libusb-1.0-0-dev
	make

	# Fedora
	dnf install gcc make flex byacc libusb-devel
	make

    # OpenBSD
	pkg_add libusb1 gmake
	gmake

s725 has been tested on the following systems:
  - Ubuntu 16.04 amd64 with both USB/FTDI/IRXON and builtin IR
  - Fedora 23 amd64 with both USB/FTDI/IRXON and builtin IR
  - Debian GNU/Linux 8.0 amd64 with USB/FTDI/IRXON
  - OpenBSD-current amd64 (May 2016) with USB/FTDI/IRXON
  - OpenBSD 5.9 sparc64 with USB/FTDI/IRXON
  - OpenBSD 5.9 hppa with serial/IRXON
  - OpenBSD 5.9 alpha with serial/IRXON

### Usage

The s725get utility takes a -d argument which specifies the driver
type to be used to communicate with the device.  Valid values are
"serial", "stir".  A device filename is required for -d values of
"serial", but not for "stir". The default is -d serial.

Examples:

	s725get -d serial -D /dev/cua00
	s725get -d serial -D /dev/ttyUSB0

You must have write permissions for the device file to use s725get.

Downloaded workout files are stored in the current directory by
default. To change this, you can pass the -f command line option to
s725get:

	s725get -f ~/workout/

s725get reads the configuration file ~/.s725rc, which might contain
the following settings:

	#
	# ~/.s725rc
	#

	# driver. possible values: serial, stir
	driver = serial

	# device file name
	device = "/dev/cuaU0"

    # output directory
    directory = "/home/user/polar/data/"

There are two ways to transfer data from the S725X watch:
Put the watch into "Connect" mode, put it next to the IR interface
and run s725get like this:

    [user@host ~]$ s725get -d serial -D /dev/ttyUSB0 -f tmp
    Reading [19163 bytes] [########################################] [  100%]
    File 01: Saved as /home/user/tmp/20160503T180809.03621.txt
    File 02: Saved as /home/user/tmp/20160501T215815.00149.txt
    File 03: Saved as /home/user/tmp/20160428T173144.01881.txt
    File 04: Saved as /home/user/tmp/20160426T181506.01677.txt
    File 05: Saved as /home/user/tmp/20160302T102256.07165.txt
    File 06: Saved as /home/user/tmp/20151103T140918.01129.txt
    File 07: Saved as /home/user/tmp/20150829T093131.03392.txt
    File 08: Saved as /home/user/tmp/20150813T180106.00149.txt
    Saved 8 files

This way the IR interface sends a "get" command and then waits for
data to arrive.  The second option is to run s725get in listen mode,
put the S725X watch into connect mode and select the file to transfer:

    [user@host ~]$ s725get -d serial -D /dev/ttyUSB0 -f tmp -l
    Reading [456 bytes] [########################################] [  100%]
    File 01: Saved as /home/user/tmp/20160501T215815.00149.txt
    Saved 1 files

### Drivers

#### serial

This driver is known to work with:
  - Serial infrared adapter IRXON SMH-IR220 attached via USB serial adapter
	http://www.irxon.com/english/products/ir220_e.htm
  - Builtin Fujitsu Lifebook T4215 IR interface in IrDA mode (see BIOS
	setup) that is accessed like a standard serial port

#### stir

This stir driver handles SigmaTel STIr4200 based USB IrDA bridges via
libusb. This means it doesn't use the Linux IrDA stack.

Current status: Sending works. Receiving always returns bit errors and
CRC errors. It looks like this device can't handle Polar communication
correctly.

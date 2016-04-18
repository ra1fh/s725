
s725
====

### About

Communicate with Polar S725X heart rate monitor via IR interfaces.
This project is based on s710 from http://code.google.com/p/s710/.

### Building

Building s725 requires libusb 1.0 or later, GNU make, flex,
yacc/byacc/bison:

	make

### Usage

The s725get utility takes a -d argument which specifies the driver
type to be used to communicate with the device.  Valid values are
"serial", "stir".  A device filename is required for -d values of
"serial", but not for "stir". The default is -d serial.

Examples:

	s725get -d serial -D /dev/cua00
	s725get -D /dev/cua00

You must have write permissions to the device file to use s725get.

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
	device = "/dev/cua01"

    # output directory
    directory = "/home/user/polar/data/"

### Drivers

#### serial

This driver is known to work with:
  - Builtin Fujitsu Lifebook T4215 IR interface in IrDA mode (see BIOS
	setup) that is accessed like a standard serial port
  - Serial infrared adapter IRXON SMH-IR220.
	http://www.irxon.com/english/products/ir220_e.htm

#### stir

This stir driver is supposed to handle a SigmaTel STIr4200 based USB IrDA
bridges. This is work in progress, not usable yet.


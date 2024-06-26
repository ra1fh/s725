[![Build](https://github.com/ra1fh/s725/actions/workflows/build.yml/badge.svg)](https://github.com/ra1fh/s725/actions/workflows/build.yml)

s725
====

### About

Communicate with Polar S725X heart rate monitor watches via IR
interfaces. This project is based on s710 from
https://code.google.com/archive/p/s710/.

Supported watches: S725X. The code for older models like S610, S710,
S625X is still present in the code and might work, but due to lack of
hardware is not tested.

### Building

Building s725 requires make, gcc/clang, flex,
yacc/byacc/bison:

	# Debian/Ubuntu
	apt-get install gcc make flex bison
	make

	# Fedora
	dnf install gcc make flex byacc
	make

    # OpenBSD
	make

s725 has been tested on the following systems:

 * Ubuntu 20.04 and 22.04 amd64 via Travis CI
 * Fedora 23 amd64 with both USB/FTDI/IRXON and builtin IR
 * Debian GNU/Linux 8.0 amd64 with USB/FTDI/IRXON
 * OpenBSD-current amd64 (May 2016) with USB/FTDI/IRXON
 * OpenBSD 5.9 sparc64 with USB/FTDI/IRXON
 * OpenBSD 5.9 hppa with serial/IRXON
 * OpenBSD 5.9 alpha with serial/IRXON

### Usage

The s725get utility downloads the data from the watch and writes it to
disk in various selectable formats: srd, hrm, tcx, txt.

The s725get utility takes a -d argument which specifies the driver
type to be used to communicate with the watch.  The only valid value
is "serial". The "serial" driver requires the device file to
be specified with -D.  The default is -d serial. See the "Drivers"
section for more details.

Examples:

	s725get -d serial -D /dev/cua00 -o tcx
	s725get -d serial -D /dev/ttyUSB0 -o hrm

You must have write permissions for the device file to use s725get.

Downloaded workout files are stored in the current directory by
default. To change this, you can pass the -f command line option to
s725get:

	s725get -f ~/workout/

s725get reads the configuration file ~/.s725rc which might contain
the following settings:

	#
	# ~/.s725rc
	#

	# driver. possible values: serial
	driver = serial

	# device file name
	device = "/dev/cuaU0"

    # output directory
    directory = "/home/user/polar/data/"

    # output format
    format = tcx

There are two ways to transfer data from the S725X watch:
Put the watch into "Connect" mode, put it next to the IR interface
and run s725get like this:

    [user@host ~]$ s725get -d serial -D /dev/ttyUSB0 -f tmp -o txt
    Reading [19163 bytes] [########################################] [  100%]
    File 01: Saved as /home/user/tmp/20160503T180809.03621.txt
    File 02: Saved as /home/user/tmp/20160501T215815.00149.txt
    File 03: Saved as /home/user/tmp/20160428T173144.01881.txt
    File 04: Saved as /home/user/tmp/20160426T181506.01677.txt
    File 05: Saved as /home/user/tmp/20160302T102256.07165.txt
    File 06: Saved as /home/user/tmp/20151103T140918.01129.txt
    File 07: Saved as /home/user/tmp/20150829T093131.03392.txt
    File 08: Saved as /home/user/tmp/20150813T180106.00149.txt

This way the IR interface sends a "get" command and then waits for
data to arrive. All training records will be downloaded.

The second option is to run s725get in listen mode. Put the S725X
watch into connect mode and select the file to transfer using the
buttons on the watch:

    [user@host ~]$ s725get -d serial -D /dev/ttyUSB0 -f tmp -o txt -l
    Reading [456 bytes] [########################################] [  100%]
    File 01: Saved as /home/user/tmp/20160501T215815.00149.txt

### Drivers

#### serial

This driver is known to work with:

 * Serial infrared adapter IRXON SMH-IR220 attached via USB serial adapter
   http://www.irxon.com/english/products/ir220_e.htm
 * Builtin Fujitsu Lifebook T4215 IR interface in IrDA mode (see BIOS
   setup) that is accessed like a standard serial port

### hrmtool

Convert Polar SRD files to HRM, TCX and TXT format. By default it tries
to auto-detect the different SRD variants (S610, S625, S725).

#### Usage

	usage: hrmtool [options] [-i intype] [-s srdversion] [-o outtype] [-f infile] [-F outfile]
	        -i intype      input file type: srd
	        -I variant     input variant: S610, S625, S725 (default: auto)
	        -o outtype     output file type: hrm, tcx, txt
	        -f infile      input file name
	        -F outfile     output file name
	        -v             verbose output

### s725plot

Script to plot heart rate over time, altitude over time and heart rate
histogram. Requires Python 2.7 or Python 3, matplotlib (tested
with 2.2) and numpy. Input has to be in 'txt' format as written by
s725get or hrmtool.

#### Usage

	Usage: s725plot <INPUT> <OUTPUT DIRECTORY>

#### Examples

![Heart Rate Diagram](examples/20160621T170047-time-hr.png)

![Altitude Diagram](examples/20160621T170047-time-alt.png)

![Speed Diagram](examples/20160621T170047-time-spd.png)

![Histogram](examples/20160621T170047-hist.png)

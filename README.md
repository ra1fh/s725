
s725
====

### About

Communicate with Polar S725X heart rate monitor via serial IR, native
IR and USB interface. It is based on s710 from
http://code.google.com/p/s710/.

### Building

Building s725 requires libusb 0.1.8 or later:

        cd src && make

### Usage

The s725get utility takes a -d argument which specifies the driver
type to be used to communicate with the device.  Valid values are
"serial", "ir".  A device filename is required for -d values of
"serial" and "ir", but not for "usb". The default is -d serial.
Examples:

        s725get -d serial /dev/ttyS0
        s725get -d ir /dev/ttyS0
        s725get -d usb
        s725get /dev/ttyS0

You must have write permissions to the device file to use s725get.

Downloaded workout files are stored in the current directory by
default. To change this, you can pass the -f command line option to
s725get:

        s725get -f ~/workout/
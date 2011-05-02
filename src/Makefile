
.PHONY: clean

VERSION= 0.1
DATE= $(shell date +%Y%m%d-%H%M)

PROGS=   s710get srdcat srdhead

PREFIX?= /usr/local

BIN_OWNER= bin
BIN_GROUP= root

CC= gcc

INCDIRS= -I$(PREFIX)/include
LDFLAGS= -L$(PREFIX)/lib

INSTALLDIR= install -d
INSTALLBIN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 555
INSTALLMAN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 444

S710GET_SRCS = driver.c files.c byte_map.c serial.c usb.c packet.c  \
	comm.c crc.c

SRDCAT_SRCS = alpha_map.c filter.c time.c \
	workout_print.c workout_read.c workout_util.c

SRDHEAD_SRCS = alpha_map.c filter.c time.c \
	workout_print.c workout_read.c workout_util.c

PROG_SRCS= $(patsubst %,%.c,$(PROGS))

LIBS+= 

DEFS+= -DS710_HRM_MAX_HR=206
DEFS+= -DS710_HRM_REST_HR=60
DEFS+= -DS710_FILEDIR=\"/var/polar/raw\"

S710GET_OBJS= $(patsubst %.c,%.o,$(S710GET_SRCS))
SRDCAT_OBJS= $(patsubst %.c,%.o,$(SRDCAT_SRCS))
SRDHEAD_OBJS= $(patsubst %.c,%.o,$(SRDHEAD_SRCS))
PROG_OBJS= $(patsubst %,%.o,$(PROGS))
CPPFLAGS+= $(DEFS) -I. $(INCDIRS)

ifeq ($(shell pkg-config --exists libusb && echo yes), yes)
	CPPFLAGS += -D HAVE_LIBUSB
	CFLAGS += $(shell pkg-config --cflags libusb)
	LIBS   += $(shell pkg-config --libs libusb)	
endif	

ifneq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
INCDIRS+= -Icompat
COMPAT_SRCS+= compat/strlcpy.c
endif

ifdef DEBUG
CFLAGS+= -g -ggdb -DDEBUG
DEFS+= -DBUILD="\"$(VERSION) ($(DATE))\""
else
DEFS+= -DBUILD="\"$(VERSION)\""
endif

COMPAT_OBJS= $(patsubst %.c,%.o,$(COMPAT_SRCS))

CFLAGS+= -Wno-long-long -Wall -W -Wnested-externs -Wformat=2
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+= -Wwrite-strings -Wshadow -Wpointer-arith -Wcast-qual -Wsign-compare
CFLAGS+= -Wundef -Wshadow -Wbad-function-cast -Winline -Wcast-align

CLEANFILES= $(S710GET_OBJS) $(SRDCAT_OBJS) $(SRDHEAD_OBJS) $(PROGS) $(PROG_OBJS) $(COMPAT_OBJS) .depend

all: $(PROGS)

s710get: s710get.o $(S710GET_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ 

srdcat: srdcat.o $(SRDCAT_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+ 

srdhead: srdhead.o $(SRDHEAD_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+

depend: $(S710GET_SRCS) $(SRDCAT_SRCS) $(SRDHEAD_SRCS)
	$(CC) $(CPPFLAGS) -MM $+ > .depend

clean:
	rm -rf $(CLEANFILES)

install:
	$(INSTALLDIR) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) $(PROGS) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) s710html $(DESTDIR)$(PREFIX)/bin

ifeq ($(wildcard .depend),.depend)
include .depend
endif


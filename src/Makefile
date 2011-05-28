
.PHONY: clean doxygen egypt

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

S710GET_SRCS = driver.c files.c serial.c usb.c packet.c  \
	utils.c xmalloc.c buf.c

SRDCAT_SRCS = label.c time_helper.c \
	workout_print.c workout_read.c workout_util.c xmalloc.c buf.c

SRDHEAD_SRCS = label.c time_helper.c \
	workout_print.c workout_read.c workout_util.c xmalloc.c buf.c

PROG_SRCS= $(patsubst %,%.c,$(PROGS))

LIBS+= 

S710GET_OBJS= $(patsubst %.c,%.o,$(S710GET_SRCS))
SRDCAT_OBJS= $(patsubst %.c,%.o,$(SRDCAT_SRCS))
SRDHEAD_OBJS= $(patsubst %.c,%.o,$(SRDHEAD_SRCS))
PROG_OBJS= $(patsubst %,%.o,$(PROGS))
CPPFLAGS+= $(DEFS) -I. $(INCDIRS)

ifeq ($(shell pkg-config --exists libusb && echo yes), yes)
	CFLAGS += $(shell pkg-config --cflags libusb)
	LIBS   += $(shell pkg-config --libs libusb)	
endif

ifneq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
INCDIRS+= -Icompat
COMPAT_SRCS+= compat/strlcpy.c
endif

ifdef DEBUG
CFLAGS+= -g -ggdb -DDEBUG
endif

COMPAT_OBJS= $(patsubst %.c,%.o,$(COMPAT_SRCS))

CFLAGS+= -Wall

ifdef EGYPT
CFLAGS+= -fdump-rtl-expand 
endif

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

doxygen:
	doxygen Doxyfile

egypt:
	egypt *.expand | dot -Tpng > callgraph.png

clean:
	rm -rf $(CLEANFILES) *.104r.expand callgraph.png

install:
	$(INSTALLDIR) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) $(PROGS) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) s710html $(DESTDIR)$(PREFIX)/bin

ifeq ($(wildcard .depend),.depend)
include .depend
endif


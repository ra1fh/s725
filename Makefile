
.PHONY: clean

PROGS=   s725get srdcat srdhead

PREFIX?= /usr/local

BIN_OWNER= bin
BIN_GROUP= root

INCDIRS= -I$(PREFIX)/include
LDFLAGS= -L$(PREFIX)/lib 

INSTALLDIR= install -d
INSTALLBIN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 555
INSTALLMAN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 444

S725GET_SRCS = driver.c files.c serial.c usb.c packet.c  \
	workout.c workout_label.c workout_time.c \
	xmalloc.c buf.c

SRDCAT_SRCS = workout_label.c workout_time.c \
	workout.c xmalloc.c buf.c

SRDHEAD_SRCS = workout_label.c workout_time.c \
	workout.c xmalloc.c buf.c

PROG_SRCS= $(patsubst %,%.c,$(PROGS))

LIBS+= $(shell pkg-config --libs glib-2.0)	

S725GET_OBJS= $(patsubst %.c,%.o,$(S725GET_SRCS))
SRDCAT_OBJS= $(patsubst %.c,%.o,$(SRDCAT_SRCS))
SRDHEAD_OBJS= $(patsubst %.c,%.o,$(SRDHEAD_SRCS))
PROG_OBJS= $(patsubst %,%.o,$(PROGS))
CPPFLAGS+= $(DEFS) -I. $(INCDIRS)

ifeq ($(shell pkg-config --exists libusb && echo yes), yes)
	CFLAGS += $(shell pkg-config --cflags libusb)
	LIBS   += $(shell pkg-config --libs libusb)	
endif

CFLAGS += $(shell pkg-config --cflags glib-2.0)
LIBS += $(shell pkg-config --libs glib-2.0)

ifneq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
INCDIRS+= -Icompat
COMPAT_SRCS+= compat/strlcpy.c
endif

ifdef DEBUG
CFLAGS+= -DDEBUG
endif

CFLAGS+= -g

COMPAT_OBJS= $(patsubst %.c,%.o,$(COMPAT_SRCS))

CFLAGS+= -Wall

CLEANFILES= $(S725GET_OBJS) $(SRDCAT_OBJS) $(SRDHEAD_OBJS) $(PROGS) $(PROG_OBJS) $(COMPAT_OBJS) .depend

all: $(PROGS)

s725get: s725get.o $(S725GET_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ 

srdcat: srdcat.o $(SRDCAT_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+ 

srdhead: srdhead.o $(SRDHEAD_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+

depend: $(S725GET_SRCS) $(SRDCAT_SRCS) $(SRDHEAD_SRCS)
	$(CC) $(CPPFLAGS) -MM $+ > .depend

clean:
	rm -rf $(CLEANFILES) *.104r.expand callgraph.png

install:
	$(INSTALLDIR) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) $(PROGS) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) s725html $(DESTDIR)$(PREFIX)/bin

ifeq ($(wildcard .depend),.depend)
include .depend
endif


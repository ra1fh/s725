
.PHONY: clean

FLEX= flex
YACC= yacc

PROGS=   s725get srdcat srdtcx srdhead

PREFIX?= /usr/local

BIN_OWNER= bin
BIN_GROUP= root

INCDIRS= -I$(PREFIX)/include
LDFLAGS= -L$(PREFIX)/lib 

INSTALLDIR= install -d
INSTALLBIN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 555
INSTALLMAN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 444

S725GET_SRCS = driver.c files.c serial.c stir.c packet.c misc.c \
	workout.c workout_label.c workout_print.c workout_time.c \
	xmalloc.c buf.c hexdump.c log.c

SRDCAT_SRCS = workout_label.c workout_time.c workout_print.c \
	workout.c xmalloc.c buf.c hexdump.c

SRDTCX_SRCS = workout_label.c workout_time.c workout_print.c \
	workout.c xmalloc.c buf.c hexdump.c

SRDHEAD_SRCS = workout_label.c workout_time.c workout_print.c \
	workout.c xmalloc.c buf.c hexdump.c

PROG_SRCS= $(patsubst %,%.c,$(PROGS))

S725GET_OBJS= $(patsubst %.c,%.o,$(S725GET_SRCS))
SRDCAT_OBJS= $(patsubst %.c,%.o,$(SRDCAT_SRCS))
SRDTCX_OBJS= $(patsubst %.c,%.o,$(SRDTCX_SRCS))
SRDHEAD_OBJS= $(patsubst %.c,%.o,$(SRDHEAD_SRCS))
PROG_OBJS= $(patsubst %,%.o,$(PROGS))
CPPFLAGS+= $(DEFS) -I. $(INCDIRS)

ifeq ($(shell pkg-config --exists libusb-1.0 && echo yes), yes)
	CFLAGS += $(shell pkg-config --cflags libusb-1.0)
	LIBS   += $(shell pkg-config --libs libusb-1.0)	
endif

ifneq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
CFLAGS+= -D_GNU_SOURCE -Icompat
COMPAT_SRCS+= compat/strlcpy.c
endif

ifdef DEBUG
CFLAGS+= -DDEBUG
endif

CFLAGS+= -g

COMPAT_OBJS= $(patsubst %.c,%.o,$(COMPAT_SRCS))

CFLAGS+= -Wall

CONF_OBJS= conf.tab.o lex.yy.o
CONF_MISC= conf.tab.c conf.tab.h lex.yy.c parser parser.o

CLEANFILES=  $(S725GET_OBJS) $(SRDCAT_OBJS) $(SRDTCX_OBJS) $(SRDHEAD_OBJS)
CLEANFILES+= $(PROGS) $(PROG_OBJS) $(COMPAT_OBJS) .depend
CLEANFILES+= $(CONF_OBJS) $(CONF_MISC)

all: $(PROGS)

s725get: s725get.o $(CONF_OBJS) $(S725GET_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+ $(LIBS) 

conf.tab.c conf.tab.h: conf.y
	$(YACC) -b conf -d conf.y

lex.yy.c: conf.l conf.tab.h
	$(FLEX) conf.l

conf.tab.o: conf.tab.c
	$(CC) -c -o conf.tab.o conf.tab.c

lex.yy.o: lex.yy.c
	$(CC) -c -o lex.yy.o lex.yy.c

parser.o: parser.c
	$(CC) -c -o parser.o parser.c

parser: $(CONF_OBJS) parser.o
	$(CC) -o parser parser.o $(CONF_OBJS)

srdcat: srdcat.o $(SRDCAT_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+ 

srdtcx: srdtcx.o $(SRDTCX_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+ 

srdhead: srdhead.o $(SRDHEAD_OBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+

depend: $(S725GET_SRCS) $(SRDCAT_SRCS) $(SRDTCX_SRCS) $(SRDHEAD_SRCS)
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


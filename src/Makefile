
.PHONY: clean

VERSION= 0.1
DATE= $(shell date +%Y%m%d-%H%M)

PROGS=   s710d s710get s710sh srd2hrm srdcat srdhead srdmerge srdplot
LIBS710= libs710.a

PREFIX?= /usr/local

BIN_OWNER= bin
BIN_GROUP= root

CC= gcc

INCDIRS= -I$(PREFIX)/include
LDFLAGS= -L$(PREFIX)/lib

INSTALLDIR= install -d
INSTALLBIN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 555
INSTALLMAN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 444

LIB_SRCS = add_attr.c alpha_map.c axes.c bike.c byte_map.c comm.c		\
	crc.c driver.c exercise.c files.c filter.c free_attr.c glob.c		\
	histogram.c logo.c merge_attr.c overview.c packet.c parse_attr.c	\
	plot.c print_attr.c reminder.c serial.c time.c transfer.c usb.c		\
	user.c watch.c workout_merge.c workout_print.c workout_read.c		\
	workout_util.c workout_write.c

S710GET_SRCS = driver.c files.c byte_map.c serial.c usb.c packet.c  \
	comm.c crc.c

SRDCAT_SRCS = alpha_map.c filter.c time.c \
	workout_print.c workout_read.c workout_util.c

SRDHEAD_SRCS = alpha_map.c filter.c time.c \
	workout_print.c workout_read.c workout_util.c

SRDPLOT_SRCS = alpha_map.c filter.c axes.c histogram.c plot.c time.c \
	workout_print.c workout_read.c workout_util.c

PROG_SRCS= $(patsubst %,%.c,$(PROGS))

LIBS+= 

DEFS+= -DS710_HRM_MAX_HR=206
DEFS+= -DS710_HRM_REST_HR=60
DEFS+= -DS710_FILEDIR=\"/var/polar/raw\"

LIB_OBJS= $(patsubst %.c,%.o,$(LIB_SRCS))
S710GET_OBJS= $(patsubst %.c,%.o,$(S710GET_SRCS))
SRDCAT_OBJS= $(patsubst %.c,%.o,$(SRDCAT_SRCS))
SRDHEAD_OBJS= $(patsubst %.c,%.o,$(SRDHEAD_SRCS))
SRDPLOT_OBJS= $(patsubst %.c,%.o,$(SRDPLOT_SRCS))
PROG_OBJS= $(patsubst %,%.o,$(PROGS))
CPPFLAGS+= $(DEFS) -I. $(INCDIRS)

ifeq ($(shell pkg-config --exists libusb && echo yes), yes)
	CPPFLAGS += -D HAVE_LIBUSB
	CFLAGS += $(shell pkg-config --cflags libusb)
	LIBS   += $(shell pkg-config --libs libusb)	
endif	

ifneq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
INCDIRS+= -Icompat
LIB_SRCS+= compat/strlcpy.c
endif

ifdef DEBUG
CFLAGS+= -g -ggdb -DDEBUG
LDFLAGS+= -rdynamic
LIBS+= -ldl
DEFS+= -DBUILD="\"$(VERSION) ($(DATE))\""
else
DEFS+= -DBUILD="\"$(VERSION)\""
endif

CFLAGS+= -Wno-long-long -Wall -W -Wnested-externs -Wformat=2
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+= -Wwrite-strings -Wshadow -Wpointer-arith -Wcast-qual -Wsign-compare
CFLAGS+= -Wundef -Wshadow -Wbad-function-cast -Winline -Wcast-align

CLEANFILES= $(LIBS710) $(LIB_OBJS) $(S710GET_OBJS) $(PROGS) $(PROG_OBJS) .depend

all: $(LIBS710) $(PROGS)

$(LIBS710): $(LIB_OBJS)
	ar cru $@ $+
	ranlib $@

s710d: s710d.o $(LIBS710)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ $(LIBS710)

s710get: s710get.o $(S710GET_OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ 

s710sh: s710sh.o $(LIBS710)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ $(LIBS710) -ledit -lncurses

srd2hrm: srd2hrm.o $(LIBS710)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ $(LIBS710)

srdcat: srdcat.o $(SRDCAT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+ 

srdhead: srdhead.o $(SRDHEAD_OBJS)
	$(CC) $(LDFLAGS) -o $@ $+

srdmerge: srdmerge.o $(LIBS710)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $+ $(LIBS710)

srdplot: srdplot.o $(SRDPLOT_OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -L/usr/X11R6/lib -o $@ $+ -lgd -lfreetype

depend: $(LIB_SRCS)
	$(CC) $(CPPFLAGS) -MM $(LIB_SRCS) > .depend

clean:
	rm -rf $(CLEANFILES)

install:
	$(INSTALLDIR) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) $(PROGS) $(DESTDIR)$(PREFIX)/bin

ifeq ($(wildcard .depend),.depend)
include .depend
endif


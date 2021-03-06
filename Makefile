
.PHONY: clean check valgrind examples

FLEX?= flex
YACC?= yacc

PREFIX?= /usr/local

BIN_OWNER= bin
BIN_GROUP= root

INCDIRS= -I$(PREFIX)/include
LDFLAGS= -L$(PREFIX)/lib

INSTALLDIR= install -d
INSTALLBIN= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 555
INSTALLDOC= install -g $(BIN_OWNER) -o $(BIN_GROUP) -m 444

PROGS= s725get hrmtool

COMMON_SRCS= workout.c workout_print.c workout_time.c \
	xmalloc.c buf.c log.c

S725GET_SRCS= $(COMMON_SRCS) s725get.c driver.c files.c format.c \
	misc.c packet.c serial.c

HRMTOOL_SRCS= $(COMMON_SRCS) hrmtool.c format.c

S725GET_OBJS= $(S725GET_SRCS:.c=.o)
HRMTOOL_OBJS= $(HRMTOOL_SRCS:.c=.o)
PROG_OBJS= $(PROGS:=.o)

CPPFLAGS+= -D_GNU_SOURCE -I. $(INCDIRS)
CFLAGS+= -g -pedantic -std=c99 -Wall

CONF_OBJS= conf.tab.o lex.yy.o

CLEANFILES= $(S725GET_OBJS) $(HRMTOOL_OBJS)
CLEANFILES+= $(PROGS) $(PROG_OBJS) .depend
CLEANFILES+= $(CONF_OBJS) conf.tab.c conf.tab.h lex.yy.c

all: $(PROGS)

conf.tab.c conf.tab.h: conf.y
	$(YACC) -b conf -d conf.y

lex.yy.c: conf.l conf.tab.h
	$(FLEX) conf.l

conf.tab.o: conf.tab.c
	$(CC) -c -o conf.tab.o conf.tab.c

lex.yy.o: lex.yy.c
	$(CC) -c -o lex.yy.o lex.yy.c

s725get: $(CONF_OBJS) $(S725GET_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CONF_OBJS) $(S725GET_OBJS)

hrmtool: $(HRMTOOL_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(HRMTOOL_OBJS)

depend: $(S725GET_SRCS) $(SRDCAT_SRCS) $(SRDTCX_SRCS) $(SRDHEAD_SRCS)
	$(CC) $(CPPFLAGS) -MM $(S725GET_SRCS) $(SRDCAT_SRCS) $(SRDTCX_SRCS) $(SRDHEAD_SRCS) > .depend

clean:
	rm -rf $(CLEANFILES)

install:
	$(INSTALLDIR) $(PREFIX)/bin
	$(INSTALLBIN) $(PROGS) $(PREFIX)/bin
	$(INSTALLBIN) s725plot $(PREFIX)/bin
	$(INSTALLDIR) $(PREFIX)/share/doc/s725
	$(INSTALLDOC) README.md $(PREFIX)/share/doc/s725/

check: hrmtool
	@cd tests && $(SHELL) runtests

valgrind: hrmtool
	@cd tests && $(SHELL) runtests "valgrind --error-exitcode=1 --leak-check=full"

examples:
	./s725plot tests/20160621T170047.txt examples/

-include .depend

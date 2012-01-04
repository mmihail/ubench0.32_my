###############################################################################
#									      #
# 				ubench Makefile				      #
#									      #
# 	       Copyright (C) Sergei A. Viznyuk, July, 1999                    #
#			      All rights reserved.			      #
#									      #
###############################################################################

#DEFINES=-DDEBUG
INSTALLDIR= /usr/local/bin

CC = gcc
CFLAGS = -O2 -Wall -malign-loops=2 -malign-jumps=2 -malign-functions=2
LDFLAGS = -s -lm
INCLUDES = -I.

objects = signals.o cpubench.o membench.o ubench.o
utimeobjects = creadok.o cwriteok.o tlock.o utime.o

ubench:	$(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(objects)
	@(strip $@)
	@(ls -l $@)

%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

default: ubench

install: ubench
	install -c -m 0555 ubench $(INSTALLDIR)

utime:	$(utimeobjects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@(strip $@)
	@(ls -l $@)

clean:
	rm -f ubench utime $(objects) $(utimeobjects) core *~

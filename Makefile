# Makefile for xtrlock - X Transparent Lock
# This Makefile provided for those of you who lack a functioning xmkmf.
#
# Copyright (C)1993,1994 Ian Jackson
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

LDLIBS=-lX11 -lcrypt
CDEFS=-DSHADOW_PWD
CC=gcc
CFLAGS=-Wall ${CDEFS} 
INSTALL=install
RM=rm

xtrlock:	xtrlock.o

xtrlock.o:	xtrlock.c

debug:
	$(CC) xtrlock.c $(LDLIBS) -Wall ${CDEFS} -D DEBUG 

install:	xtrlock
	echo "install -c -m 2755 -o root -g shadow xtrlock \$$1" > file_to_dest.sh;
	chmod +x ./file_to_dest.sh;
	echo $(PATH)| awk -F ":" '{$$(1)=$$(1); print $$(0)}'|xargs ./file_to_dest.sh;
	rm file_to_dest.sh;

install.man:
	$(INSTALL) -c -m 644 xtrlock.man /usr/man/man1/xtrlock.1x

remove:
	echo "cd \$$1 && rm xtrlock" > rm_file_to_dest.sh;
	chmod +x ./rm_file_to_dest.sh;
	echo $(PATH) | awk -F ":" '{$$1=$$1; print $$0}'|xargs ./rm_file_to_dest.sh;
	rm rm_file_to_dest.sh;



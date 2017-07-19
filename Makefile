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

LDLIBS=-lX11 -lcrypt $(shell pkg-config --libs libnotify)
CDEFS=-DSHADOW_PWD $(RESPATH)
CC=gcc
CFLAGS=-Wall ${CDEFS} $(shell pkg-config --cflags libnotify)
INSTALL=install
RM=rm
CONFIGPATH=/usr/share/xtrlock/
RESPATH=-D'LOCK_IMG_PATH="$(CONFIGPATH)lock.png"' -D'UNLOCK_IMG_PATH="$(CONFIGPATH)unlock.png"'
#RESPATH=-D'LOCK_IMG_PATH="$(shell readlink -f lock.png)"' -D'UNLOCK_IMG_PATH="$(shell readlink -f unlock.png)"'

xtrlock:	xtrlock.o

xtrlock.o:	xtrlock.c

debug:
	$(CC) xtrlock.c $(LDLIBS) $(CFLAGS) -DDEBUG -g -o xtrlock

clean:
	-rm -f xtrlock.o xtrlock

install:	xtrlock
		$(INSTALL) -c -m 2755 -o root -g shadow xtrlock /usr/bin
	$(INSTALL) -c -m 2755 -o root -g shadow xtrlock /usr/bin
	if [ ! -d "$(CONFIGPATH)" ]; then mkdir $(CONFIGPATH); fi
	$(INSTALL) -c -m 644 resources/lock.png $(CONFIGPATH)
	$(INSTALL) -c -m 644 resources/unlock.png $(CONFIGPATH)

install.man:
		$(INSTALL) -c -m 644 xtrlock.man /usr/man/man1/xtrlock.1x


remove:
	$(RM) /usr/bin/xtrlock
	$(RM) -rf $(CONFIGPATH)

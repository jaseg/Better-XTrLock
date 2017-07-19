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
LID_CMD:=xtrlock -l

xtrlock:	xtrlock.o

xtrlock.o:	xtrlock.c

debug:
	$(CC) xtrlock.c $(LDLIBS) $(CFLAGS) -DDEBUG -g -o xtrlock

clean:
	-rm -f xtrlock.o xtrlock

install:	xtrlock
	$(INSTALL) -c -m 2755 -o root -g shadow xtrlock /usr/bin
	if [ ! -d "$(CONFIGPATH)" ]; then mkdir $(CONFIGPATH); fi
	$(INSTALL) -c -m 644 resources/lock.png $(CONFIGPATH)
	$(INSTALL) -c -m 644 resources/unlock.png $(CONFIGPATH)

install.man:
	$(INSTALL) -c -m 644 xtrlock.man /usr/share/man/man1/xtrlock.1x

install.bash_completion:
	$(INSTALL) -c -m 754 xtrlock-completion.sh /usr/share/bash-completion/completions/xtrlock

install.on_lid:
	cp ./on-lid-close.sh on-lid-close.sh.tmp
	sed 's/xtrlock -l/$(LID_CMD)/g' on-lid-close.sh.tmp > tmp && mv tmp on-lid-close.sh.tmp
	$(INSTALL) -c -m 744 -o root xtrlock-lid-down /etc/acpi/events/xtrlock-lid-down
	$(INSTALL) -c -m 744 -o root ./on-lid-close.sh.tmp /etc/acpi/on-lid-close.sh
	rm -f on-lid-close.sh.tmp
remove:
	$(RM) /usr/bin/xtrlock
	$(RM) -rf $(CONFIGPATH)
	$(RM) -f /usr/share/bash-completion/completions/xtrlock
	$(RM) -f /etc/acpi/on-lid-close.sh /etc/acpi/events/xtrlock-lid-down

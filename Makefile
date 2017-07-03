#!/usr/bin/make -f
# Copyright (C) 2017 David Michael <fedora.dm0@gmail.com>
#
# This file is part of the Hurd rump audio translator.
#
# The Hurd rump audio translator is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# The Hurd rump audio translator is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# the Hurd rump audio translator.  If not, see <http://www.gnu.org/licenses/>.

CC ?= cc
CPP ?= $(CC) -E
INSTALL_DATA ?= install -Dpm 0644
INSTALL_PROGRAM ?= install -Dpm 0755
MIG ?= mig
RM ?= rm -f

prefix ?= /usr
hurddir ?= $(exec_prefix)/hurd
includedir ?= $(prefix)/include

DEFS := ioctl-audio.defs
SRCS := $(DEFS:.defs=.c) ioctl.c main.c trivfs.c
LIBS := -lports -lrumpclient -lshouldbeinlibc -ltrivfs

all: audio audio.h
install: all
	$(INSTALL_PROGRAM) audio $(DESTDIR)$(hurddir)/audio
	$(INSTALL_DATA) audio.h $(DESTDIR)$(includedir)/sys/audio.h
clean:
	$(RM) $(DEFS:.defs=.c) $(DEFS:.defs=.h) $(SRCS:.c=.o) audio
.PHONY: all clean install

audio: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c $(DEFS:.defs=.h)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

# Generate the ioctl server code with MIG.  Client code is built into glibc.
ioctl-%.c ioctl-%.h: ioctl-%.defs
	$(CPP) $(CPPFLAGS) -x c -o - $< | \
	$(MIG) -cc cat - /dev/null -subrprefix __ \
		-header $(<:.defs=.h) -server $(<:.defs=.c) -user /dev/null

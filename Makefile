# cproxy - Copyright 2012 Aaron Riekenberg (aaron.riekenberg@gmail.com).
#
# cproxy is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cproxy is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cproxy.  If not, see <http://www.gnu.org/licenses/>.

CC = gcc
CFLAGS = -pthread -g -O3 -Wall
LDFLAGS = -pthread

SRC = bufferpool.c \
      errutil.c \
      fdutil.c \
      linkedlist.c \
      log.c \
      memutil.c \
      pollutil.c \
      pollresult.c \
      proxy.c \
      rb.c \
      socketutil.c \
      sortedtable.c \
      timeutil.c
OBJS = $(SRC:.c=.o)

all: cproxy

clean:
	rm -f *.o cproxy

cproxy: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

depend:
	$(CC) $(CFLAGS) -MM $(SRC) > .makeinclude

include .makeinclude

CC = gcc
CFLAGS = -pthread -g -O3 -Wall
LDFLAGS = -pthread

SRC = bufferpool.c \
      errutil.c \
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

all: proxy

clean:
	rm -f *.o proxy

proxy: $(OBJS)

depend:
	$(CC) $(CFLAGS) -MM $(SRC) > .makeinclude

include .makeinclude

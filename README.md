# C Proxy

## Description
TCP proxy implemented in C.

## Theory of Operation
* 1 acceptor thread to accept incoming client connections.
* Pool of 1 to N I/O threads to handle read, write, and connect operations.  Pool size configurable with -t option.  Client sessions assigned to I/O threads using round robin.
* 2 buffers per client session, one for each direction of traffic.  Buffer size configurable with -b option.  Buffers are allocated from per-thread buffer pools.
* All sockets are non-blocking.  All read, write, connect, and accept operations are asynchronous.
* Automatically chooses between epoll, kqueue, and poll as the poll system call.  epoll or kqueue are recommended because they allow storing pointers to connection state information in the kernel.  If poll is used, connection state information is stored in a red-black tree from libavl 2.0.3.
* kqueue is currently only supported on FreeBSD because that's the only platform I have access to test.  It should also work on OS X and other BSDs.
#ifndef SOCKETUTIL_H
#define SOCKETUTIL_H

#include <netdb.h>
#include <sys/socket.h>

struct AddrPortStrings
{
  char addrString[NI_MAXHOST];
  char portString[NI_MAXSERV];
};

extern int addressToNameAndPort(
  const struct sockaddr* address,
  socklen_t addressSize,
  struct AddrPortStrings* addrPortStrings);

extern int setSocketListening(
  int socket);

extern int setSocketNonBlocking(
  int socket);

extern int setSocketReuseAddress(
  int socket);

extern int setSocketNoDelay(
  int socket);

extern int getSocketError(
  int socket);

extern int signalSafeAccept(
  int sockfd,
  struct sockaddr* addr,
  socklen_t* addrlen);

extern ssize_t signalSafeRead(
  int fd,
  void* buf,
  size_t count);

extern ssize_t signalSafeWrite(
  int fd,
  void* buf,
  size_t count);

extern int signalSafeClose(
  int fd);

#endif

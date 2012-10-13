#include "socketutil.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int addressToNameAndPort(
  const struct sockaddr* address,
  socklen_t addressSize,
  struct AddrPortStrings* addrPortStrings)
{
  int retVal;
  if ((retVal = getnameinfo(address,
                            addressSize,
                            addrPortStrings->addrString,
                            NI_MAXHOST,
                            addrPortStrings->portString,
                            NI_MAXSERV,
                            (NI_NUMERICHOST | NI_NUMERICSERV))) != 0)
  {
    printf("getnameinfo error: %s\n", gai_strerror(retVal));
    return -1;
  }
  return 0;
}

int setSocketListening(
  int socket)
{
  return listen(socket, SOMAXCONN);
}

int setSocketNonBlocking(
  int socket)
{
  int flags = fcntl(socket, F_GETFL, 0);
  if (flags < 0)
  {
    return flags;
  }
  flags |= O_NONBLOCK;
  return fcntl(socket, F_SETFL, flags);
}

int setSocketReuseAddress(
  int socket)
{
  int optval = 1;
  return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

int setSocketNoDelay(
  int socket)
{
  int optval = 1;
  return setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

int getSocketError(
  int socket)
{
  int optval = 0;
  socklen_t optlen = sizeof(optval);
  int retval = 
    getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval, &optlen);
  if (retval < 0)
  {
    return retval;
  }
  return optval;
}

int signalSafeAccept(
  int sockfd,
  struct sockaddr* addr,
  socklen_t* addrlen)
{
  bool interrupted;
  int retVal;
  do
  {
    retVal = accept(sockfd, addr, addrlen);
    interrupted =
      ((retVal < 0) &&
       (errno == EINTR));
  } while (interrupted);
  return retVal;
}

ssize_t signalSafeRead(
  int fd,
  void* buf,
  size_t count)
{
  bool interrupted;
  ssize_t retVal;
  do
  {
    retVal = read(fd, buf, count);
    interrupted =
      ((retVal < 0) &&
       (errno == EINTR));
  } while (interrupted);
  return retVal;
}

ssize_t signalSafeWrite(
  int fd,
  void* buf,
  size_t count)
{
  bool interrupted;
  ssize_t retVal;
  do
  {
    retVal = write(fd, buf, count);
    interrupted =
      ((retVal < 0) &&
       (errno == EINTR));
  } while (interrupted);
  return retVal;
}

int signalSafeClose(
  int fd)
{
  bool interrupted;
  int retVal;
  do
  {
    retVal = close(fd);
    interrupted =
      ((retVal < 0) &&
       (errno == EINTR));
  } while (interrupted);
  return retVal;
}

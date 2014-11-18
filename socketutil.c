/* cproxy - Copyright (C) 2012 Aaron Riekenberg (aaron.riekenberg@gmail.com).

   cproxy is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   cproxy is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with cproxy.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "socketutil.h"
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
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
  int retVal =
    getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval, &optlen);
  if (retVal < 0)
  {
    return retVal;
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

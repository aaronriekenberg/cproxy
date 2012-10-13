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

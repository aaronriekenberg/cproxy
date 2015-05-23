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

#ifndef FDUTIL_H
#define FDUTIL_H

#include <stddef.h>

extern int setFDNonBlocking(
  int fd);

enum ReadFromFDStatus
{
  READ_FROM_FD_WOULD_BLOCK,
  READ_FROM_FD_EOF,
  READ_FROM_FD_ERROR,
  READ_FROM_FD_SUCCESS
};

struct ReadFromFDResult
{
  enum ReadFromFDStatus status;
  int readErrno;
  size_t bytesRead;
};

struct ReadFromFDResult readFromFD(
  int fd,
  void* buf,
  size_t bytesToRead);

enum WriteToFDStatus
{
  WRITE_TO_FD_WOULD_BLOCK,
  WRITE_TO_FD_ERROR,
  WRITE_TO_FD_SUCCESS
};

struct WriteToFDResult
{
  enum WriteToFDStatus status;
  int writeErrno;
  size_t bytesWritten;
};

struct WriteToFDResult writeToFD(
  int fd,
  const void* buf,
  size_t bytesToWrite);

extern int signalSafeClose(
  int fd);

#endif

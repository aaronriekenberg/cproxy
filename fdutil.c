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

#include "fdutil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>

int setFDNonBlocking(
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

static ssize_t signalSafeRead(
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

struct ReadFromFDResult readFromFD(
  int fd,
  void* buf,
  size_t bytesToRead)
{
  ssize_t readRetVal;

  assert(fd >= 0);
  assert(buf != NULL);

  readRetVal = signalSafeRead(fd, buf, bytesToRead);
  if ((readRetVal == -1) &&
      ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
  {
    const struct ReadFromFDResult result =
    {
      .status = READ_FROM_FD_WOULD_BLOCK,
      .bytesRead = 0,
      .readErrno = errno
    };
    return result;
  }
  else if (readRetVal == 0)
  {
    const struct ReadFromFDResult result =
    {
      .status = READ_FROM_FD_EOF,
      .bytesRead = 0,
      .readErrno = 0
    };
    return result;
  }
  else if (readRetVal == -1)
  {
    const struct ReadFromFDResult result =
    {
      .status = READ_FROM_FD_ERROR,
      .bytesRead = 0,
      .readErrno = errno
    };
    return result;
  }
  else
  {
    const struct ReadFromFDResult result =
    {
      .status = READ_FROM_FD_SUCCESS,
      .bytesRead = readRetVal,
      .readErrno = 0
    };
    return result;
  }
}

static ssize_t signalSafeWrite(
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

struct WriteToFDResult writeToFD(
  int fd,
  void* buf,
  size_t bytesToWrite)
{
  ssize_t writeRetVal;

  assert(fd >= 0);
  assert(buf != NULL);

  writeRetVal = signalSafeWrite(fd, buf, bytesToWrite);
  if ((writeRetVal == -1) &&
      ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
  {
    const struct WriteToFDResult result =
    {
      .status = WRITE_TO_FD_WOULD_BLOCK,
      .bytesWritten = 0,
      .writeErrno = errno
    };
    return result;
  }
  else if (writeRetVal == -1)
  {
    const struct WriteToFDResult result =
    {
      .status = WRITE_TO_FD_ERROR,
      .bytesWritten = 0,
      .writeErrno = errno
    };
    return result;
  }
  else
  {
    const struct WriteToFDResult result =
    {
      .status = WRITE_TO_FD_SUCCESS,
      .bytesWritten = writeRetVal,
      .writeErrno = 0
    };
    return result;
  }
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

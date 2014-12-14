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

#include "errutil.h"
#include "log.h"
#include "memutil.h"
#include "pollutil.h"
#include "sortedtable.h"
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

struct InternalPollState
{
  size_t numFDs;
  struct pollfd* pollfdArray;
  struct SortedTable fdDataTable;
};

void initializePollState(
  struct PollState* pollState)
{
  struct InternalPollState* internalPollState;

  assert(pollState != NULL);

  memset(pollState, 0, sizeof(struct PollState));
  internalPollState =
    checkedCalloc(1, sizeof(struct InternalPollState));
  pollState->internalPollState = internalPollState;
  initializeSortedTable(&(internalPollState->fdDataTable));
  proxyLog("created poll");
}

void addPollFDToPollState(
  struct PollState* pollState,
  int fd,
  void* data,
  enum ReadEventInterest readEventInterest,
  enum WriteEventInterest writeEventInterest)
{
  struct InternalPollState* internalPollState;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  if (sortedTableContainsKey(
        &(internalPollState->fdDataTable),
        fd))
  {
    proxyLog("attempt to add duplicate fd %d to PollState",
             fd);
    abort();
  }

  ++(internalPollState->numFDs);
  addToSortedTable(
    &(internalPollState->fdDataTable),
    fd, data);
  internalPollState->pollfdArray =
    checkedRealloc(internalPollState->pollfdArray,
                   internalPollState->numFDs * sizeof(struct pollfd));
  internalPollState->pollfdArray[internalPollState->numFDs - 1].fd = fd;
  internalPollState->pollfdArray[internalPollState->numFDs - 1].events =
    ((readEventInterest == INTERESTED_IN_READ_EVENTS) ? POLLIN : 0) |
    ((writeEventInterest == INTERESTED_IN_WRITE_EVENTS) ? POLLOUT : 0);
}

void updatePollFDInPollState(
  struct PollState* pollState,
  int fd,
  void* data,
  enum ReadEventInterest readEventInterest,
  enum WriteEventInterest writeEventInterest)
{
  struct InternalPollState* internalPollState;
  size_t i;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  if (!sortedTableContainsKey(
         &(internalPollState->fdDataTable),
         fd))
  {
    proxyLog("attempt to update unknown fd %d in PollState",
             fd);
    abort();
  }

  replaceDataInSortedTable(
    &(internalPollState->fdDataTable),
    fd, data);

  for (i = 0; i < internalPollState->numFDs; ++i)
  {
    if (internalPollState->pollfdArray[i].fd == fd)
    {
      internalPollState->pollfdArray[i].events =
        ((readEventInterest == INTERESTED_IN_READ_EVENTS) ? POLLIN : 0) |
        ((writeEventInterest == INTERESTED_IN_WRITE_EVENTS) ? POLLOUT : 0);
      break;
    }
  }
}

void removePollFDFromPollState(
  struct PollState* pollState,
  int fd)
{
  struct InternalPollState* internalPollState;
  struct pollfd* newPollfdArray;
  size_t newPollfdArrayIndex;
  size_t i;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  if (!sortedTableContainsKey(
         &(internalPollState->fdDataTable),
         fd))
  {
    proxyLog("attempt to remove unknown fd %d from PollState",
             fd);
    abort();
  }

  newPollfdArray =
    checkedMalloc((internalPollState->numFDs - 1) * sizeof(struct pollfd));
  newPollfdArrayIndex = 0;
  for (i = 0; i < internalPollState->numFDs; ++i)
  {
    if (internalPollState->pollfdArray[i].fd != fd)
    {
      newPollfdArray[newPollfdArrayIndex].fd =
        internalPollState->pollfdArray[i].fd;
      newPollfdArray[newPollfdArrayIndex].events =
        internalPollState->pollfdArray[i].events;
      ++newPollfdArrayIndex;
    }
  }

  free(internalPollState->pollfdArray);
  internalPollState->pollfdArray = newPollfdArray;
  --(internalPollState->numFDs);
  removeFromSortedTable(
    &(internalPollState->fdDataTable),
    fd);
}

static int signalSafePoll(
  struct pollfd* fds,
  nfds_t nfds,
  int timeout)
{
  bool interrupted;
  int retVal;
  do
  {
    retVal = poll(fds, nfds, timeout);
    interrupted =
      ((retVal < 0) &&
       (errno == EINTR));
    timeout = 0;
  } while (interrupted);
  return retVal;
}

const struct PollResult* blockingPoll(
  struct PollState* pollState)
{
  struct InternalPollState* internalPollState;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  if (internalPollState->numFDs > 0)
  {
    size_t i;
    size_t readyFDIndex = 0;
    int retVal = 
      signalSafePoll(
        internalPollState->pollfdArray,
        internalPollState->numFDs,
        -1);
    if (retVal < 0)
    {
      proxyLog("poll error errno %d: %s",
               errno,
               errnoToString(errno));
      abort();
    }
    setPollResultNumReadyFDs(
      &(pollState->pollResult),
      retVal);
    for (i = 0; i < internalPollState->numFDs; ++i)
    {
      const struct pollfd* readyPollFD =
        &(internalPollState->pollfdArray[i]);
      if (readyPollFD->revents)
      {
        struct ReadyFDInfo* readyFDInfo =
          &(pollState->pollResult.readyFDInfoArray[readyFDIndex]);
        readyFDInfo->data =
          findDataInSortedTable(
            &(internalPollState->fdDataTable),
            readyPollFD->fd);
        readyFDInfo->readyForRead =
          ((readyPollFD->revents & POLLIN) != 0);
        readyFDInfo->readyForWrite =
          ((readyPollFD->revents & POLLOUT) != 0);
        readyFDInfo->readyForError =
          ((readyPollFD->revents & (POLLERR | POLLHUP | POLLNVAL)) != 0);
        ++readyFDIndex;
      }
    }
    return (&(pollState->pollResult));
  }
  return NULL;
}

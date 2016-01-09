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
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>

struct InternalPollState
{
  int    epollFD;
  size_t numFDs;
  struct epoll_event* epollEventArray;
  size_t epollEventArrayCapacity;
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
  internalPollState->epollFD = epoll_create1(0);
  if (internalPollState->epollFD < 0)
  {
    proxyLog("epoll_create1 error errno %d: %s",
             errno,
             errnoToString(errno));
    abort();
  }
  proxyLog("created epoll (fd=%d)",
           internalPollState->epollFD);
}

void addPollFDToPollState(
  struct PollState* pollState,
  int fd,
  void* data,
  enum ReadEventInterest readEventInterest,
  enum WriteEventInterest writeEventInterest)
{
  struct InternalPollState* internalPollState;
  struct epoll_event newEvent;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  newEvent.data.ptr = data;
  newEvent.events = 
    ((readEventInterest == INTERESTED_IN_READ_EVENTS) ? EPOLLIN : 0) |
    ((writeEventInterest == INTERESTED_IN_WRITE_EVENTS) ? EPOLLOUT : 0);
  if (epoll_ctl(internalPollState->epollFD,
                EPOLL_CTL_ADD,
                fd,
                &newEvent) < 0)
  {
    proxyLog("epoll_ctl(EPOLL_CTL_ADD) error fd %d errno %d: %s",
             fd,
             errno,
             errnoToString(errno));
    abort();
  }
  else
  {
    bool changedCapacity = false;
    ++(internalPollState->numFDs);
    while (internalPollState->numFDs >
           internalPollState->epollEventArrayCapacity)
    {
      changedCapacity = true;
      if (internalPollState->epollEventArrayCapacity == 0)
      {
        internalPollState->epollEventArrayCapacity = 16;
      }
      else
      {
        internalPollState->epollEventArrayCapacity *= 2;
      }
    }
    if (changedCapacity)
    {
      internalPollState->epollEventArray =
        checkedRealloc(internalPollState->epollEventArray,
                       internalPollState->epollEventArrayCapacity *
                       sizeof(struct epoll_event));
    }
  }
}


void updatePollFDInPollState(
  struct PollState* pollState,
  int fd,
  void* data,
  enum ReadEventInterest readEventInterest,
  enum WriteEventInterest writeEventInterest)
{
  struct InternalPollState* internalPollState;
  struct epoll_event newEvent;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  newEvent.data.ptr = data;
  newEvent.events =
    ((readEventInterest == INTERESTED_IN_READ_EVENTS) ? EPOLLIN : 0) |
    ((writeEventInterest == INTERESTED_IN_WRITE_EVENTS) ? EPOLLOUT : 0);
  if (epoll_ctl(internalPollState->epollFD,
                EPOLL_CTL_MOD,
                fd,
                &newEvent) < 0)
  {
    proxyLog("epoll_ctl(EPOLL_CTL_MOD) error fd %d errno %d: %s",
             fd,
             errno,
             errnoToString(errno));
    abort();
  }
}

void removePollFDFromPollState(
  struct PollState* pollState,
  int fd)
{
  struct InternalPollState* internalPollState;

  assert(pollState != NULL);

  internalPollState = pollState->internalPollState;
  if (epoll_ctl(internalPollState->epollFD,
                EPOLL_CTL_DEL,
                fd,
                NULL) < 0)
  {
    proxyLog("epoll_ctl(EPOLL_CTL_DEL) error fd %d errno %d: %s",
             fd,
             errno,
             errnoToString(errno));
    abort();
  }
  else
  {
    --(internalPollState->numFDs);
  }
}

static int signalSafeEpollWait(
  int epfd,
  struct epoll_event* events,
  int maxevents,
  int timeout)
{
  bool interrupted;
  int retVal;
  do
  {
    retVal = epoll_wait(epfd, events, maxevents, timeout);
    interrupted = ((retVal < 0) &&
                   (errno == EINTR));
    /* Set timeout = 0 so that if the last epoll_wait was
       interrupted we don't block on the next try.  Assumes
       waiting too long is worse than not waiting long enough. */
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
    const int retVal = 
      signalSafeEpollWait(
        internalPollState->epollFD,
        internalPollState->epollEventArray,
        internalPollState->numFDs,
        -1);
    if (retVal < 0)
    {
      proxyLog("epoll_wait error errno %d: %s",
               errno,
               errnoToString(errno));
      abort();
    }
    setPollResultNumReadyFDs(
      &(pollState->pollResult),
      retVal);
    for (i = 0; i < retVal; ++i)
    {
      struct ReadyFDInfo* readyFDInfo = 
        &(pollState->pollResult.readyFDInfoArray[i]);
      const struct epoll_event* readyEpollEvent =
        &(internalPollState->epollEventArray[i]);
      readyFDInfo->data = readyEpollEvent->data.ptr;
      readyFDInfo->readyForRead =
        ((readyEpollEvent->events & EPOLLIN) != 0);
      readyFDInfo->readyForWrite =
        ((readyEpollEvent->events & EPOLLOUT) != 0);
      readyFDInfo->readyForError =
        ((readyEpollEvent->events & (EPOLLERR | EPOLLHUP)) != 0);
    }
    return (&(pollState->pollResult));
  }
  return NULL;
}

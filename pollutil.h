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

#ifndef POLLUTIL_H
#define POLLUTIL_H

#include "pollresult.h"

struct PollState
{
  void* internalPollState;
  struct PollResult pollResult;
};

extern void initializePollState(
  struct PollState* pollState);

enum ReadEventInterest
{
  INTERESTED_IN_READ_EVENTS,
  NOT_INTERESTED_IN_READ_EVENTS,
};

enum WriteEventInterest
{
  INTERESTED_IN_WRITE_EVENTS,
  NOT_INTERESTED_IN_WRITE_EVENTS,
};

/* Add fd to PollState. */
extern void addPollFDToPollState(
  struct PollState* pollState,
  int fd,
  void* data,
  enum ReadEventInterest readEventInterest,
  enum WriteEventInterest writeEventInterest);

/* Update fd that has been previously added to PollState. */
extern void updatePollFDInPollState(
  struct PollState* pollState,
  int fd,
  void* data,
  enum ReadEventInterest readEventInterest,
  enum WriteEventInterest writeEventInterest);

/* Remove fd from PollState. */
extern void removePollFDFromPollState(
  struct PollState* pollState,
  int fd);

extern const struct PollResult* blockingPoll(
  struct PollState* pollState);

#endif

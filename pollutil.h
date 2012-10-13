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

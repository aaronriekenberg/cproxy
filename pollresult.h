#ifndef POLLRESULT_H
#define POLLRESULT_H

#include <stdbool.h>
#include <stddef.h>

struct ReadyFDInfo
{
  void* data;
  bool readyForRead;
  bool readyForWrite;
  bool readyForError;
};

struct PollResult
{
  size_t numReadyFDs;
  struct ReadyFDInfo* readyFDInfoArray;
  size_t arrayCapacity;
};

extern void setPollResultNumReadyFDs(
  struct PollResult* pollResult,
  size_t numReadyFDs);

#endif

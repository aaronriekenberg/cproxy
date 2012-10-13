#include <assert.h>
#include <stdbool.h>
#include "memutil.h"
#include "pollresult.h"

void setPollResultNumReadyFDs(
  struct PollResult* pollResult,
  size_t numReadyFDs)
{
  bool changedCapacity = false;

  assert(pollResult != NULL);

  while (numReadyFDs > pollResult->arrayCapacity)
  {
    changedCapacity = true;
    if (pollResult->arrayCapacity == 0)
    {
      pollResult->arrayCapacity = 16;
    }
    else
    {
      pollResult->arrayCapacity *= 2;
    }
  }

  if (changedCapacity)
  {
    pollResult->readyFDInfoArray =
      checkedRealloc(
        pollResult->readyFDInfoArray,
        pollResult->arrayCapacity * sizeof(struct ReadyFDInfo),
        __FILE__, __LINE__);
  }

  pollResult->numReadyFDs = numReadyFDs;
}

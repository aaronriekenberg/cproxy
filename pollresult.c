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
        pollResult->arrayCapacity * sizeof(struct ReadyFDInfo));
  }

  pollResult->numReadyFDs = numReadyFDs;
}

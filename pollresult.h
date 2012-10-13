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

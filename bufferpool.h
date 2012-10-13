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

#ifndef BUFFERPOOL_H
#define BUFFERPOOL_H

#include <stddef.h>

struct BufferPool
{
  size_t bufferSize;
  size_t poolSize;
  size_t buffersInPool;
  void** bufferArray;
};

extern void initializeBufferPool(
  struct BufferPool* bufferPool,
  size_t bufferSize,
  size_t initialPoolSize);

extern void* getBufferFromBufferPool(
  struct BufferPool* bufferPool);

extern void returnBufferToBufferPool(
  struct BufferPool* bufferPool,
  void* buffer);

#endif

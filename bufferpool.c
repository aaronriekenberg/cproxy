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

#include "bufferpool.h"
#include "memutil.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void resizeBufferPool(
  struct BufferPool* bufferPool,
  size_t newSize)
{
  size_t originalSize;
  size_t i;

  assert(bufferPool != NULL);
  assert(newSize > bufferPool->poolSize);

  originalSize = bufferPool->poolSize;
  bufferPool->poolSize = newSize;
  bufferPool->bufferArray = 
    checkedRealloc(bufferPool->bufferArray,
                   newSize * sizeof(void*));
  memset(
    &(bufferPool->bufferArray[originalSize]),
    0,
    (newSize - originalSize) * sizeof(void*));

  for (i = 0; i < (newSize - originalSize); ++i)
  {
    void* buffer = checkedMalloc(bufferPool->bufferSize);
    returnBufferToBufferPool(
      bufferPool,
      buffer);
  }
}

void initializeBufferPool(
  struct BufferPool* bufferPool,
  size_t bufferSize,
  size_t initialPoolSize)
{
  assert(bufferPool != NULL);
  assert(bufferSize > 0);
  assert(initialPoolSize > 0);

  memset(bufferPool, 0, sizeof(struct BufferPool));

  bufferPool->bufferSize = bufferSize;
  resizeBufferPool(bufferPool, initialPoolSize);
}

void* getBufferFromBufferPool(
  struct BufferPool* bufferPool)
{
  void* retVal;

  assert(bufferPool != NULL);

  if (bufferPool->buffersInPool == 0)
  {
    resizeBufferPool(
      bufferPool,
      bufferPool->poolSize * 2);
  }

  retVal = bufferPool->bufferArray[bufferPool->buffersInPool - 1];
  bufferPool->bufferArray[bufferPool->buffersInPool - 1] = NULL;
  --(bufferPool->buffersInPool);

  return retVal;
}

void returnBufferToBufferPool(
  struct BufferPool* bufferPool,
  void* buffer)
{
  assert(bufferPool != NULL);
  assert(bufferPool->buffersInPool < bufferPool->poolSize);

  bufferPool->bufferArray[bufferPool->buffersInPool] = buffer;
  ++(bufferPool->buffersInPool);
}

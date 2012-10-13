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

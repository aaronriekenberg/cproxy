#ifndef MEMUTIL_H
#define MEMUTIL_H

#include <stdlib.h>

extern void* checkedMalloc(
  size_t size,
  const char* file,
  int line);

extern void* checkedCalloc(
  size_t nmemb,
  size_t size,
  const char* file,
  int line);

extern void* checkedRealloc(
  void* ptr,
  size_t size,
  const char* file,
  int line);

#endif

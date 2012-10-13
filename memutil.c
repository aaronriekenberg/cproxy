#include "memutil.h"
#include <stdio.h>

void* checkedMalloc(
  size_t size,
  const char* file,
  int line)
{
  void* retVal = malloc(size);
  if ((!retVal) && size)
  {
    printf("malloc failed size %ld %s:%d\n",
           (long)size, file, line);
    abort();
  }
  return retVal;
}

void* checkedCalloc(
  size_t nmemb,
  size_t size,
  const char* file,
  int line)
{
  void* retVal = calloc(nmemb, size);
  if ((!retVal) && nmemb && size)
  {
    printf("calloc failed nmemb %ld size %ld %s:%d\n",
           (long)nmemb, (long)size, file, line);
    abort();
  }
  return retVal;
}

void* checkedRealloc(
  void* ptr,
  size_t size,
  const char* file,
  int line)
{
  void* retVal = realloc(ptr, size);
  if ((!retVal) && size)
  {
    printf("realloc failed ptr %p size %ld %s:%d\n",
           ptr, (long)size, file, line);
    abort();
  }
  return retVal;
}

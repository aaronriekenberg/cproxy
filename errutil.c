#include "errutil.h"
#include "memutil.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* errnoToString(int errnoToTranslate)
{
  char* buffer = checkedMalloc(80, __FILE__, __LINE__);
  int retVal = strerror_r(errnoToTranslate, buffer, 80);
  if (retVal < 0)
  {
    printf("strerror_r error retVal = %d errnoToTranslate = %d errno = %d\n",
           retVal, errnoToTranslate, errno);
    abort();
  }
  return buffer;
}

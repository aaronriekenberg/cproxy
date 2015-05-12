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

#include "log.h"
#include "memutil.h"
#include "timeutil.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static bool logThreadNameKeyCreated = false;

static pthread_key_t logThreadNameKey;

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

static void lockLogMutex()
{
  const int retVal = pthread_mutex_lock(&logMutex);
  if (retVal != 0)
  {
    printf("pthread_mutex_lock error %d\n", retVal);
    abort();
  }
}

static void unlockLogMutex()
{
  const int retVal = pthread_mutex_unlock(&logMutex);
  if (retVal != 0)
  {
    printf("pthread_mutex_unlock error %d\n", retVal);
    abort();
  }
}

static void freeLogThreadName(void* ptr)
{
  free(ptr);
}

/* Must hold logMutex while calling. */
static void initializeLogThreadNameKey()
{
  if (!logThreadNameKeyCreated)
  {
    const int retVal =
      pthread_key_create(&logThreadNameKey, &freeLogThreadName);
    if (retVal != 0)
    {
      printf("pthread_key_create error %d\n", retVal);
      abort();
    }
    logThreadNameKeyCreated = true;
  }
}

/* Must hold logMutex while calling. */
static char* getLogThreadName()
{
  char* ptr;

  initializeLogThreadNameKey();

  ptr = pthread_getspecific(logThreadNameKey);
  if (ptr)
  {
    return ptr;
  }
  else
  {
    return "Unknown";
  }
}

void proxyLogSetThreadName(const char* threadName)
{
  int retVal;
  void* oldDataPtr;
  char* threadNameCopy;

  lockLogMutex();

  initializeLogThreadNameKey();

  unlockLogMutex();

  oldDataPtr = pthread_getspecific(logThreadNameKey);
  if (oldDataPtr)
  {
    freeLogThreadName(oldDataPtr);
    oldDataPtr = NULL;
  }

  threadNameCopy = strdup(threadName);
  if (!threadNameCopy)
  {
    printf("strdup failed\n");
    abort();
  }

  retVal = pthread_setspecific(logThreadNameKey, threadNameCopy);
  if (retVal != 0)
  {
    printf("pthread_setspecific error %d\n", retVal);
    abort();
  }
}

void proxyLog(const char* format, ...)
{
  va_list args;

  va_start(args, format);

  lockLogMutex();

  printTimeString();
  printf(" [%s] ", getLogThreadName());
  vprintf(format, args);
  printf("\n");
  fflush(stdout);

  unlockLogMutex();

  va_end(args);
}

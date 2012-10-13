#include "timeutil.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

void printTimeString()
{
  int retVal;
  char buffer[80];
  struct timeval tv;
  struct tm tm;

  if (gettimeofday(&tv, NULL) < 0)
  {
    printf("gettimeofday error\n");
    abort();
  }

  if (!localtime_r(&tv.tv_sec, &tm))
  {
    printf("localtime_r error\n");
    abort();
  }

  retVal = strftime(buffer, 80, "%Y-%b-%d %H:%M:%S", &tm);
  if (retVal <= 0)
  {
    printf("strftime error\n");
    abort();
  }
  else if (retVal > (80 - 7 - 1))
  {
    printf("strftime overflow\n");
    abort();
  }
  else
  {
    sprintf(&buffer[retVal], ".%06ld", (long)tv.tv_usec);
  }

  printf("%s", buffer);
}

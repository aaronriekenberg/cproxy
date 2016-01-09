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

#include "timeutil.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

void printTimeString()
{
  size_t charsWritten;
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

  charsWritten = strftime(buffer, 80, "%Y-%b-%d %H:%M:%S", &tm);
  if (charsWritten == 0)
  {
    printf("strftime error\n");
    abort();
  }
  else if (charsWritten > (80 - 7 - 1))
  {
    printf("strftime overflow\n");
    abort();
  }

  snprintf(&buffer[charsWritten], 8, ".%06ld", (long)tv.tv_usec);
  printf("%s", buffer);
}

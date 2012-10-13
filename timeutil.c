/* cproxy - Copyright 2012 Aaron Riekenberg (aaron.riekenberg@gmail.com).

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

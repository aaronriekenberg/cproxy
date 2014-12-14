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

#include "memutil.h"
#include <stdio.h>

void* checkedMalloc(
  size_t size)
{
  void* retVal = malloc(size);
  if ((!retVal) && size)
  {
    printf("malloc failed size %ld\n",
           (long)size);
    abort();
  }
  return retVal;
}

void* checkedCalloc(
  size_t nmemb,
  size_t size)
{
  void* retVal = calloc(nmemb, size);
  if ((!retVal) && nmemb && size)
  {
    printf("calloc failed nmemb %ld size %ld\n",
           (long)nmemb, (long)size);
    abort();
  }
  return retVal;
}

void* checkedRealloc(
  void* ptr,
  size_t size)
{
  void* retVal = realloc(ptr, size);
  if ((!retVal) && size)
  {
    printf("realloc failed ptr %p size %ld\n",
           ptr, (long)size);
    abort();
  }
  return retVal;
}

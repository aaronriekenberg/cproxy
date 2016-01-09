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

#include "errutil.h"
#include "memutil.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* errnoToString(int errnoToTranslate)
{
  char* buffer = checkedMalloc(80);
  const int retVal = strerror_r(errnoToTranslate, buffer, 80);
  if (retVal < 0)
  {
    printf("strerror_r error retVal = %d errnoToTranslate = %d errno = %d\n",
           retVal, errnoToTranslate, errno);
    abort();
  }
  return buffer;
}

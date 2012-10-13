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

#include <assert.h>
#include <stdio.h>
#include "memutil.h"
#include "rb.h"
#include "sortedtable.h"

struct SortedTableEntry
{
  int key;
  void* data;
};

static int compareSortedTableEntries(
  const void* p1, const void* p2, void* rbParam)
{
  const struct SortedTableEntry* ste1 = p1;
  const struct SortedTableEntry* ste2 = p2;

  if (ste1->key < ste2->key)
  {
    return -1;
  }
  else if (ste1->key == ste2->key)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

static void* sortedTableTreeMalloc(
  struct libavl_allocator* allocator,
  size_t size)
{
  return checkedMalloc(size, __FILE__, __LINE__);
}

static void sortedTableTreeFree(
  struct libavl_allocator* allocator,
  void* ptr)
{
  free(ptr);
}

static struct libavl_allocator sortedTableTreeAllocator =
{
  sortedTableTreeMalloc,
  sortedTableTreeFree
};

void initializeSortedTable(
  struct SortedTable* sortedTable)
{
  assert(sortedTable != NULL);

  sortedTable->pRBTable =
    rb_create(&compareSortedTableEntries, NULL, &sortedTableTreeAllocator);
}

void addToSortedTable(
  struct SortedTable* sortedTable,
  int key,
  void* data)
{
  struct SortedTableEntry* pEntry;

  assert(sortedTable != NULL);

  pEntry = checkedMalloc(sizeof(struct SortedTableEntry), __FILE__, __LINE__);
  pEntry->key = key;
  pEntry->data = data;

  if (rb_insert(sortedTable->pRBTable, pEntry))
  {
    printf("addToSortedTable attempt to insert duplicate key %d\n", key);
    abort();
  }
}

void* replaceDataInSortedTable(
  struct SortedTable* sortedTable,
  int key,
  void* data)
{
  void* oldData = NULL;
  struct SortedTableEntry* pReplacedEntry;
  struct SortedTableEntry* pEntry;

  assert(sortedTable != NULL);

  pEntry = checkedMalloc(sizeof(struct SortedTableEntry), __FILE__, __LINE__);
  pEntry->key = key;
  pEntry->data = data;

  pReplacedEntry = rb_replace(sortedTable->pRBTable, pEntry);
  if (pReplacedEntry)
  {
    oldData = pReplacedEntry->data;
    free(pReplacedEntry);
  }

  return oldData;
}

bool sortedTableContainsKey(
  const struct SortedTable* sortedTable,
  int key)
{
  bool retVal = false;
  struct SortedTableEntry entryToFind = {key, NULL};

  assert(sortedTable != NULL);

  if (rb_find(sortedTable->pRBTable, &entryToFind))
  {
    retVal = true;
  }

  return retVal;
}

void* findDataInSortedTable(
  const struct SortedTable* sortedTable,
  int key)
{
  void* retVal = NULL;
  struct SortedTableEntry entryToFind = {key, NULL};
  struct SortedTableEntry* pEntryFound;

  assert(sortedTable != NULL);

  pEntryFound = rb_find(sortedTable->pRBTable, &entryToFind);
  if (pEntryFound)
  {
    retVal = pEntryFound->data;
  }
  return retVal;
}

void* removeFromSortedTable(
  struct SortedTable* sortedTable,
  int key)
{
  void* retVal = NULL;
  struct SortedTableEntry entryToFind = {key, NULL};
  struct SortedTableEntry* pEntryFound;

  assert(sortedTable != NULL);

  pEntryFound = rb_delete(sortedTable->pRBTable, &entryToFind);
  if (pEntryFound)
  {
    retVal = pEntryFound->data;
    free(pEntryFound);
  }
  return retVal;
}

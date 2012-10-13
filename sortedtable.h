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

#ifndef SORTED_TABLE_H
#define SORTED_TABLE_H

#include <stdbool.h>

struct SortedTable
{
  void* pRBTable;
};

extern void initializeSortedTable(
  struct SortedTable* sortedTable);

/* Add (key, data) to sorted table.
 * Calls abort() if key is already in table. */
extern void addToSortedTable(
  struct SortedTable* sortedTable,
  int key,
  void* data);

/* Add (key, data) to sorted table.
 * Replaces old (key, data) if present in table.
 * Returns old data if replaced, otherwise NULL. */
extern void* replaceDataInSortedTable(
  struct SortedTable* sortedTable,
  int key,
  void* data);

/* Return non-zero if found.
 * Return 0 if not found. */
extern bool sortedTableContainsKey(
  const struct SortedTable* sortedTable,
  int key);

/* Return data if found.
 * Return NULL if not found. */
extern void* findDataInSortedTable(
  const struct SortedTable* sortedTable,
  int key);

/* Remove (key, data) and return data if found.
 * Return NULL if not found. */
extern void* removeFromSortedTable(
  struct SortedTable* sortedTable,
  int key);

#endif

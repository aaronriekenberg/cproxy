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

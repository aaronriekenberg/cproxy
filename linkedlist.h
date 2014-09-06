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

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>

struct LinkedListNode
{
  void* data;
  struct LinkedListNode* prev;
  struct LinkedListNode* next;
};

struct LinkedList
{
  struct LinkedListNode* head;
  struct LinkedListNode* tail;
  size_t size;
};

extern void initializeLinkedList(
  struct LinkedList* list);

#define EMPTY_LINKED_LIST {.head = NULL, .tail = NULL, .size = 0}

/* Add new LinkedListNode to the tail of the list with specified data */
extern void addToLinkedList(
  struct LinkedList* list,
  void* data);

/* Remove first LinkedListNode in the list with specified data */
extern void removeFromLinkedList(
  struct LinkedList* list,
  void* data);

#endif

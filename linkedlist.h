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

#define EMPTY_LINKED_LIST {NULL, NULL, 0}

/* Add new LinkedListNode to the tail of the list with specified data */
extern void addToLinkedList(
  struct LinkedList* list,
  void* data);

/* Remove first LinkedListNode in the list with specified data */
extern void removeFromLinkedList(
  struct LinkedList* list,
  void* data);

#endif

#include <assert.h>
#include "linkedlist.h"
#include "memutil.h"
#include <string.h>

void initializeLinkedList(
  struct LinkedList* list)
{
  assert(list != NULL);

  memset(list, 0, sizeof(struct LinkedList));
}

void addToLinkedList(
  struct LinkedList* list,
  void* data)
{
  struct LinkedListNode* node;

  assert(list != NULL);

  node = checkedCalloc(1, sizeof(struct LinkedListNode), __FILE__, __LINE__);
  node->data = data;
  if (list->tail == NULL)
  {
    list->head = node;
    list->tail = node;
  }
  else
  {
    list->tail->next = node;
    node->prev = list->tail;
    list->tail = node;
  }
  ++(list->size);
}

void removeFromLinkedList(
  struct LinkedList* list,
  void* data)
{
  struct LinkedListNode* currentNode;

  assert(list != NULL);

  for (currentNode = list->head;
       currentNode;
       currentNode = currentNode->next)
  {
    if (currentNode->data == data)
    {
      break;
    }
  }

  if (currentNode)
  {
    if (list->head == currentNode)
    {
      list->head = currentNode->next;
    }
    if (list->tail == currentNode)
    {
      list->tail = currentNode->prev;
    }
    if (currentNode->prev)
    {
      currentNode->prev->next = currentNode->next;
    }
    if (currentNode->next)
    {
      currentNode->next->prev = currentNode->prev;
    }
    free(currentNode);
    --(list->size);
  }
}

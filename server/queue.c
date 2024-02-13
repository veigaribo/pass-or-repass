#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

void queue_init(struct queue *dest) {
  dest->front = NULL;
  dest->back = NULL;
}

bool queue_is_empty(struct queue *q) { return q->front == NULL; }

void queue_enqueue(struct queue *q, void *value) {
  struct list *new_front = malloc((sizeof(struct list)));
  struct list *old_front = q->front;

  new_front->data = value;
  new_front->next = old_front;
  new_front->previous = NULL;

  if (old_front != NULL) {
    old_front->previous = new_front;
  }

  if (q->back == NULL) {
    q->back = new_front;
  }

  q->front = new_front;
}

void *queue_dequeue(struct queue *q) {
  if (q->back == NULL) return NULL;

  struct list *old_back = q->back;
  void *value = old_back->data;

  if (old_back->previous == (void *)0x20) {
    printf("!!!!!!!!!!!!!!BACK 0x20!!!!!!!!!!!!!!!!!\n");
  }

  struct list *new_back = old_back->previous;

  q->back = new_back;

  if (new_back == NULL) {
    q->front = NULL;
  } else {
    new_back->next = NULL;
  }

  free(old_back);

  return value;
}

void queue_free(struct queue *q, bool free_values) {
  list_free(q->front, free_values);
}

#ifndef H_PASS_OR_REPASS_QUEUE
#define H_PASS_OR_REPASS_QUEUE

#include <stdbool.h>

#include "list.h"

struct queue {
  struct list *front;
  struct list *back;
};

void queue_init(struct queue *dest);
bool queue_is_empty(struct queue *q);
void queue_enqueue(struct queue *q, void *value);
void *queue_dequeue(struct queue *q);
void queue_free(struct queue *q, bool free_values);

#endif

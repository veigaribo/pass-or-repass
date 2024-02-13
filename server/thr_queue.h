#ifndef H_PASS_OR_REPASS_THR_QUEUE
#define H_PASS_OR_REPASS_THR_QUEUE

#include <pthread.h>
#include <stdbool.h>

#include "queue.h"

struct thr_queue {
  struct queue data;

  pthread_mutex_t mutex;
  bool destroyed;
};

void thr_queue_init(struct thr_queue *dest);
bool thr_queue_is_empty(struct thr_queue *q);
void thr_queue_enqueue(struct thr_queue *q, void *value);
void *thr_queue_dequeue(struct thr_queue *q);
void thr_queue_free(struct thr_queue *q, bool free_values);

#endif

#include "thr_queue.h"

#include <stdlib.h>

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

void thr_queue_init(struct thr_queue *dest) {
  // pthread_mutex_lock(&global_mutex);

  queue_init(&dest->data);
  pthread_mutex_init(&dest->mutex, NULL);
  dest->destroyed = false;

  // pthread_mutex_unlock(&global_mutex);
}

bool thr_queue_is_empty(struct thr_queue *q) {
  pthread_mutex_lock(&q->mutex);

  return queue_is_empty(&q->data);

  pthread_mutex_unlock(&q->mutex);
}

void thr_queue_enqueue(struct thr_queue *q, void *value) {
  pthread_mutex_lock(&q->mutex);

  queue_enqueue(&q->data, value);

  pthread_mutex_unlock(&q->mutex);
}

void *thr_queue_dequeue(struct thr_queue *q) {
  pthread_mutex_lock(&q->mutex);

  void *value = queue_dequeue(&q->data);

  pthread_mutex_unlock(&q->mutex);

  return value;
}

void thr_queue_free(struct thr_queue *q, bool free_values) {
  pthread_mutex_lock(&global_mutex);

  if (q->destroyed)
    return;

  q->destroyed = true;

  pthread_mutex_destroy(&q->mutex);
  queue_free(&q->data, free_values);

  pthread_mutex_unlock(&global_mutex);
}

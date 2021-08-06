#include "thr_queue.h"

#include <stdlib.h>

void thr_queue_init(struct thr_queue *dest) {
  struct queue *data = malloc(sizeof(struct queue));
  queue_init(data);

  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);

  dest->data = data;
  dest->mutex = mutex;
}

bool thr_queue_is_empty(struct thr_queue *q) {
  pthread_mutex_lock(q->mutex);

  return queue_is_empty(q->data);

  pthread_mutex_unlock(q->mutex);
}

void thr_queue_enqueue(struct thr_queue *q, void *value) {
  pthread_mutex_lock(q->mutex);

  queue_enqueue(q->data, value);

  pthread_mutex_unlock(q->mutex);
}

void *thr_queue_dequeue(struct thr_queue *q) {
  pthread_mutex_lock(q->mutex);

  void *value = queue_dequeue(q->data);

  pthread_mutex_unlock(q->mutex);

  return value;
}

void thr_queue_free(struct thr_queue *q, bool free_values) {
  pthread_mutex_lock(q->mutex);

  queue_free(q->data, free_values);

  pthread_mutex_unlock(q->mutex);

  pthread_mutex_destroy(q->mutex);

  free(q->data);
  free(q->mutex);
}

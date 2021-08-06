#include "thr_vector.h"

#include <pthread.h>
#include <stdlib.h>

void thr_vector_init(struct thr_vector *dest) {
  struct vector *data = malloc(sizeof(struct vector));
  vector_init(data, 0, 32);

  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);

  dest->data = data;
  dest->mutex = mutex;
}

void thr_vector_grow(struct thr_vector *q) {
  pthread_mutex_lock(q->mutex);

  vector_grow(q->data);

  pthread_mutex_unlock(q->mutex);
}

void thr_vector_fit(struct thr_vector *vec, int length) {
  pthread_mutex_lock(vec->mutex);

  vector_fit(vec->data, length);

  pthread_mutex_unlock(vec->mutex);
}

void thr_vector_push(struct thr_vector *vec, void *data) {
  pthread_mutex_lock(vec->mutex);

  vector_push(vec->data, data);

  pthread_mutex_unlock(vec->mutex);
}

void *thr_vector_rm_by_addr(struct thr_vector *vec, void *addr) {
  void *result;

  pthread_mutex_lock(vec->mutex);

  result = vector_rm_by_addr(vec->data, addr);

  pthread_mutex_unlock(vec->mutex);

  return result;
}

void *thr_vector_pop(struct thr_vector *vec, int index) {
  void *result;

  pthread_mutex_lock(vec->mutex);

  result = vector_pop(vec->data, index);

  pthread_mutex_unlock(vec->mutex);

  return result;
}

void thr_vector_free(struct thr_vector *q) {
  pthread_mutex_lock(q->mutex);

  free(q->mutex);
  vector_free(q->data);

  pthread_mutex_unlock(q->mutex);

  pthread_mutex_destroy(q->mutex);

  free(q->data);
  free(q->mutex);
}

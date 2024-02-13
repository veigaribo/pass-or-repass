#include "thr_vector.h"

#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

void thr_vector_init(struct thr_vector *dest) {
  // pthread_mutex_lock(&global_mutex);

  // TODO
  vector_init(&dest->data, 0, 32);
  pthread_mutex_init(&dest->mutex, NULL);
  dest->destroyed = false;

  // pthread_mutex_unlock(&global_mutex);
}

void thr_vector_grow(struct thr_vector *q) {
  pthread_mutex_lock(&q->mutex);

  vector_grow(&q->data);

  pthread_mutex_unlock(&q->mutex);
}

void thr_vector_fit(struct thr_vector *vec, int length) {
  pthread_mutex_lock(&vec->mutex);

  vector_fit(&vec->data, length);

  pthread_mutex_unlock(&vec->mutex);
}

void thr_vector_push(struct thr_vector *vec, void *data) {
  pthread_mutex_lock(&vec->mutex);

  vector_push(&vec->data, data);

  pthread_mutex_unlock(&vec->mutex);
}

void *thr_vector_rm_by_addr(struct thr_vector *vec, void *addr) {
  void *result;

  pthread_mutex_lock(&vec->mutex);

  result = vector_rm_by_addr(&vec->data, addr);

  pthread_mutex_unlock(&vec->mutex);

  return result;
}

void *thr_vector_pop(struct thr_vector *vec, int index) {
  void *result;

  pthread_mutex_lock(&vec->mutex);

  result = vector_pop(&vec->data, index);

  pthread_mutex_unlock(&vec->mutex);

  return result;
}

void thr_vector_free(struct thr_vector *vec) {
  pthread_mutex_lock(&global_mutex);

  if (vec->destroyed)
    return;

  vec->destroyed = true;

  pthread_mutex_destroy(&vec->mutex);
  vector_free(&vec->data);

  pthread_mutex_unlock(&global_mutex);
}

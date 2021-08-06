#ifndef H_PASS_OR_REPASS_THR_VECTOR
#define H_PASS_OR_REPASS_THR_VECTOR

#include <pthread.h>

#include "vector.h"

// Thread-safe
struct thr_vector {
  struct vector *data;
  pthread_mutex_t *mutex;
};

void thr_vector_init(struct thr_vector *dest);
void thr_vector_grow(struct thr_vector *q);
void thr_vector_fit(struct thr_vector *q, int length);
void thr_vector_push(struct thr_vector *vec, void *data);
void *thr_vector_rm_by_addr(struct thr_vector *vec, void *addr);
void *thr_vector_pop(struct thr_vector *vec, int index);
void thr_vector_free(struct thr_vector *q);

#endif
